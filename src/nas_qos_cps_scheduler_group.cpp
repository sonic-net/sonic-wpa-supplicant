
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*!
 * \file   nas_qos_cps_scheduler_group.cpp
 * \brief  NAS qos scheduler_group related CPS API routines
 */

#include "cps_api_events.h"
#include "cps_api_operation.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"

#include "event_log_types.h"
#include "event_log.h"
#include "std_error_codes.h"
#include "std_mutex_lock.h"

#include "nas_qos_common.h"
#include "nas_qos_switch_list.h"
#include "nas_qos_cps.h"
#include "cps_api_key.h"
#include "dell-base-qos.h"
#include "nas_qos_scheduler_group.h"
#include "nas_switch.h"
#include "hal_if_mapping.h"
#include "nas_qos_cps_queue.h"

#define R_1_0_DISABLE_SG_CREATE

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_scheduler_group &scheduler_group);
static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                const nas::attr_set_t attr_set,
                                                const nas_qos_scheduler_group &scheduler_group);
static nas_qos_scheduler_group * nas_qos_cps_get_scheduler_group(uint_t switch_id,
                                           nas_obj_id_t scheduler_group_id);
static cps_api_return_code_t nas_qos_cps_api_scheduler_group_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_scheduler_group_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_scheduler_group_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);

static std_mutex_lock_create_static_init_rec(scheduler_group_mutex);

#define MAX_SG_CHILD_NUM 64

static t_std_error create_port_scheduler_group(hal_ifindex_t port_id,
                                    ndi_obj_id_t ndi_sg_id,
                                    const ndi_qos_scheduler_group_struct_t& sg_info,
                                    ndi_obj_id_t& nas_sg_id)
{
   interface_ctrl_t intf_ctrl;

    if (nas_qos_get_port_intf(port_id, &intf_ctrl) != true) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Cannot find NPU id for ifIndex: %d",
                      port_id);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_switch *switch_p = nas_qos_get_switch_by_npu(intf_ctrl.npu_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "switch_id of ifindex: %u cannot be found/created",
                     port_id);
        return NAS_QOS_E_FAIL;
    }

    try {
        // create the scheduler-group and add it to switch
        nas_sg_id = switch_p->alloc_scheduler_group_id();
        nas_qos_scheduler_group sg(switch_p);

        sg.set_scheduler_group_id(nas_sg_id);
        sg.add_npu(intf_ctrl.npu_id);
        sg.set_port_id(port_id);
        sg.set_ndi_obj_id(intf_ctrl.npu_id, ndi_sg_id);
        sg.set_level(sg_info.level);
        sg.clear_all_dirty_flags();
        sg.mark_ndi_created();

        switch_p->add_scheduler_group(sg);
    }
    catch (...) {
        return NAS_QOS_E_FAIL;
    }

    return STD_ERR_OK;
}

typedef std::unordered_map<ndi_obj_id_t, std::vector<ndi_obj_id_t>> sg_info_list_t;

static t_std_error create_port_scheduler_group_list(const std::vector<ndi_obj_id_t> ndi_sg_id_list,
                                                    npu_id_t npu_id,
                                                    hal_ifindex_t ifindex,
                                                    sg_info_list_t& sg_info_list)
{
    ndi_qos_scheduler_group_struct_t sg_info;
    ndi_obj_id_t child_id_list[MAX_SG_CHILD_NUM];
    nas_attr_id_t attr_list[] = {
        BASE_QOS_SCHEDULER_GROUP_CHILD_COUNT,
        BASE_QOS_SCHEDULER_GROUP_CHILD_LIST,
        BASE_QOS_SCHEDULER_GROUP_PORT_ID,
        BASE_QOS_SCHEDULER_GROUP_LEVEL,
        BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID
    };
    t_std_error rc;
    ndi_obj_id_t nas_sg_id;

    for (auto ndi_sg_id: ndi_sg_id_list) {
        memset(&sg_info, 0, sizeof(sg_info));
        sg_info.child_count = sizeof(child_id_list) / sizeof(child_id_list[0]);
        sg_info.child_list = child_id_list;
        rc = ndi_qos_get_scheduler_group(npu_id, ndi_sg_id,
                                         attr_list, sizeof(attr_list)/sizeof(attr_list[0]),
                                         &sg_info);
        if (rc != STD_ERR_OK) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                         "Failed to get attributes of scheduler-group %lu", ndi_sg_id);
            return rc;
        }
        rc = create_port_scheduler_group(ifindex, ndi_sg_id, sg_info, nas_sg_id);
        if (rc != STD_ERR_OK) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                         "Failed to create scheduler-group object");
            return rc;
        }
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Created scheduler-group object: ndi_id 0x%lx nas_id 0x%lx level %d",
                     ndi_sg_id, nas_sg_id, sg_info.level);
        std::vector<ndi_obj_id_t> child_list;
        for (uint_t j = 0; j < sg_info.child_count; j ++) {
            child_list.push_back(sg_info.child_list[j]);
        }
        sg_info_list[nas_sg_id] = std::move(child_list);
    }

    return STD_ERR_OK;
}

static t_std_error setup_scheduler_group_hierarchy(npu_id_t npu_id,
                                                   const sg_info_list_t& sg_info_list)
{
    // Setup child list for each scheduler-group object
    ndi_obj_id_t nas_sg_id, child_id;
    nas_qos_switch *switch_p = nas_qos_get_switch_by_npu(npu_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "switch_id of npu_id: %u cannot be found/created",
                     npu_id);
        return NAS_QOS_E_FAIL;
    }
    for (auto& it: sg_info_list) {
        nas_sg_id = it.first;
        nas_qos_scheduler_group *sg_p = switch_p->get_scheduler_group(nas_sg_id);
        if (!sg_p) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                         "Couldn't find scheduler-group object for id %lu", nas_sg_id);
            return NAS_QOS_E_FAIL;
        }
        for (uint_t idx = 0; idx < it.second.size(); idx ++) {
            if (sg_p->next_level_is_queue()) {
                nas_qos_queue* queue_p = switch_p->get_queue_by_id(it.second[idx]);
                if (!queue_p) {
                    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                                 "Couldn't find child queue, ndi_id 0x%lx",
                                 it.second[idx]);
                    return NAS_QOS_E_FAIL;
                }
                queue_p->set_queue_parent_id(sg_p->get_scheduler_group_id());
                child_id = queue_p->get_queue_id();
            } else {
                nas_qos_scheduler_group* child_sg_p =
                            switch_p->get_scheduler_group_by_id(npu_id,
                                                                it.second[idx]);
                if (!child_sg_p) {
                    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                                 "Couldn't find child scheduler-group, ndi_id 0x%lx",
                                 it.second[idx]);
                    return NAS_QOS_E_FAIL;
                }
                child_sg_p->set_scheduler_group_parent_id(sg_p->get_scheduler_group_id());
                child_id = child_sg_p->get_scheduler_group_id();
            }
            sg_p->add_child(child_id);
        }

        // clear the dirty flag because this information is read from SAI, not going to SAI
        sg_p->clear_all_dirty_flags();
    }
    return STD_ERR_OK;
}

t_std_error nas_qos_port_scheduler_group_init(hal_ifindex_t ifindex)
{
    interface_ctrl_t intf_ctrl;

    if (nas_qos_port_queue_init(ifindex) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Failed to initiate queue for port %lu", ifindex);
        return NAS_QOS_E_FAIL;
    }

    if (nas_qos_get_port_intf(ifindex, &intf_ctrl) == false) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Cannot find ifindex %u",
                     ifindex);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_switch *switch_p = nas_qos_get_switch_by_npu(intf_ctrl.npu_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "switch_id of npu_id: %u cannot be found/created",
                     intf_ctrl.npu_id);
        return NAS_QOS_E_FAIL;
    }

    if (switch_p->port_sg_is_initialized(ifindex)) {
        return STD_ERR_OK; // initialized
    }

    ndi_port_t ndi_port_id;
    ndi_port_id.npu_id = intf_ctrl.npu_id;
    ndi_port_id.npu_port = intf_ctrl.port_id;

    /* get the number of queues per port */
    uint_t sg_count = ndi_qos_get_number_of_scheduler_groups(ndi_port_id);
    if (sg_count == 0) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "No scheduler-group for npu_id %u, npu_port_id %u ",
                     ndi_port_id.npu_id, ndi_port_id.npu_port);
        return STD_ERR_OK;
    }

    /* get the list of ndi_queue id list */
    std::vector<ndi_obj_id_t> ndi_sg_id_list(sg_count);
    if (ndi_qos_get_scheduler_group_id_list(ndi_port_id, sg_count, &ndi_sg_id_list[0]) !=
            sg_count) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Fail to retrieve all scheduler-groups of npu_id %u, npu_port_id %u ",
                     ndi_port_id.npu_id, ndi_port_id.npu_port);
        return NAS_QOS_E_FAIL;
    }

    t_std_error rc;
    sg_info_list_t sg_info_list;
    rc = create_port_scheduler_group_list(ndi_sg_id_list, intf_ctrl.npu_id, ifindex,
                                          sg_info_list);
    if (rc != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Failed to create scheduler-groups for default hierarchy");
        return rc;
    }
    rc = setup_scheduler_group_hierarchy(intf_ctrl.npu_id, sg_info_list);
    if (rc != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Failed to setup scheduler-group hierarchy");
        return rc;
    }

    return STD_ERR_OK;
}

/**
  * This function provides NAS-QoS SCHEDULER_GROUP CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_scheduler_group_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&scheduler_group_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
        return nas_qos_cps_api_scheduler_group_create(obj, param->prev);

    case cps_api_oper_SET:
        return nas_qos_cps_api_scheduler_group_set(obj, param->prev);

    case cps_api_oper_DELETE:
        return nas_qos_cps_api_scheduler_group_delete(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}


/**
  * This function provides NAS-QoS SCHEDULER_GROUP CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_scheduler_group_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t sg_attr = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID);
    uint32_t port_id = 0, level;
    bool have_port = false, have_level = false;
    if (!sg_attr) {
        cps_api_object_it_t it;
        cps_api_object_it_begin(obj, &it);
        for ( ; cps_api_object_it_valid(&it); cps_api_object_it_next(&it)) {
            cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
            if (id == BASE_QOS_SCHEDULER_GROUP_PORT_ID) {
                port_id = cps_api_object_attr_data_u32(it.attr);
                have_port = true;
            } else if (id == BASE_QOS_SCHEDULER_GROUP_LEVEL) {
                level = cps_api_object_attr_data_u32(it.attr);
                have_level = true;
            }
        }
    }

    if (!sg_attr && !have_port) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                     "Invalid input attributes for reading");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint_t switch_id = 0;
    nas_obj_id_t scheduler_group_id = (sg_attr? cps_api_object_attr_data_u64(sg_attr): 0);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, scheduler_group id 0x%016lX\n",
                    switch_id, scheduler_group_id);

    std_mutex_simple_lock_guard p_m(&scheduler_group_mutex);

    if (have_port) {
        nas_qos_port_scheduler_group_init(port_id);
    }
    nas_qos_switch * p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    std::vector<nas_qos_scheduler_group *> sg_list;
    if (sg_attr) {
        nas_qos_scheduler_group *scheduler_group = p_switch->get_scheduler_group(scheduler_group_id);
        if (scheduler_group == NULL) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                         "Could not find scheduler group with ID %llu", scheduler_group_id);
            return NAS_QOS_E_FAIL;
        }
        sg_list.push_back(scheduler_group);
    } else {
        int match_level = have_level ? (int)level : -1;
        p_switch->get_port_scheduler_groups(port_id, match_level, sg_list);
    }

    /* fill in data */
    cps_api_object_t ret_obj;

    for (auto scheduler_group: sg_list) {
        ret_obj = cps_api_object_list_create_obj_and_append (param->list);
        if (ret_obj == NULL) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                         "Failed to create cps object");
            return cps_api_ret_code_ERR;
        }

        cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),BASE_QOS_SCHEDULER_GROUP_OBJ,
                cps_api_qualifier_TARGET);
        cps_api_set_key_data(ret_obj, BASE_QOS_SCHEDULER_GROUP_SWITCH_ID,
                cps_api_object_ATTR_T_U32,
                &switch_id, sizeof(uint32_t));
        scheduler_group_id = scheduler_group->get_scheduler_group_id();
        cps_api_set_key_data(ret_obj, BASE_QOS_SCHEDULER_GROUP_ID,
                cps_api_object_ATTR_T_U64,
                &scheduler_group_id, sizeof(uint64_t));

        cps_api_object_attr_add_u32(ret_obj,
                                    BASE_QOS_SCHEDULER_GROUP_CHILD_COUNT,
                                    scheduler_group->get_child_count());

        for (uint32_t idx = 0; idx < scheduler_group->get_child_count(); idx++) {
            cps_api_object_attr_add_u64(ret_obj,
                                        BASE_QOS_SCHEDULER_GROUP_CHILD_LIST,
                                        scheduler_group->get_child_id(idx));
        }

        cps_api_object_attr_add_u32(ret_obj, BASE_QOS_SCHEDULER_GROUP_PORT_ID,
                                    scheduler_group->get_port_id());
        cps_api_object_attr_add_u32(ret_obj, BASE_QOS_SCHEDULER_GROUP_LEVEL,
                                    scheduler_group->get_level());
        cps_api_object_attr_add_u64(ret_obj, BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID,
                                    scheduler_group->get_scheduler_profile_id());
    }

    return cps_api_ret_code_OK;
}


/**
  * This function provides NAS-QoS SCHEDULER_GROUP CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_scheduler_group_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&scheduler_group_mutex);

    if (op == cps_api_oper_CREATE) {
        nas_qos_cps_api_scheduler_group_delete(obj, NULL);
    }

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_scheduler_group_set(obj, NULL);
    }

    if (op == cps_api_oper_DELETE) {
        nas_qos_cps_api_scheduler_group_create(obj, NULL);
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_scheduler_group_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{
#ifndef R_1_0_DISABLE_SG_CREATE
    nas_obj_id_t scheduler_group_id = 0;

    cps_api_object_attr_t switch_attr = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_GROUP_SWITCH_ID);
    if (switch_attr == NULL ) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " switch id not specified in the message\n");
        return NAS_QOS_E_FAIL;
    }

    uint_t switch_id = cps_api_object_attr_data_u32(switch_attr);

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    nas_qos_scheduler_group scheduler_group(p_switch);

    if (nas_qos_cps_parse_attr(obj, scheduler_group) != cps_api_ret_code_OK)
        return NAS_QOS_E_FAIL;

    if (scheduler_group.get_level() == 0 &&
        scheduler_group.dirty_attr_list().contains(BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID)) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                " Scheduler Group Level 0 node does not accept Scheduler Profile ID; "
                " Use Port Egress Object to configure Scheduler Profile!\n");
        return NAS_QOS_E_FAIL;
    }

    try {
        scheduler_group_id = p_switch->alloc_scheduler_group_id();

        scheduler_group.set_scheduler_group_id(scheduler_group_id);

        scheduler_group.commit_create(sav_obj? false: true);

        p_switch->add_scheduler_group(scheduler_group);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Created new scheduler_group %u\n",
                     scheduler_group.get_scheduler_group_id());

        // update obj with new scheduler_group-id key
        cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID,
                cps_api_object_ATTR_T_U64,
                &scheduler_group_id, sizeof(uint64_t));

        // save for rollback if caller requests it.
        if (sav_obj) {
            cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (!tmp_obj) {
                return NAS_QOS_E_FAIL;
            }
            cps_api_object_clone(tmp_obj, obj);

        }

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS SCHEDULER_GROUP Create error code: %d ",
                    e.err_code);
        if (scheduler_group_id)
            p_switch->release_scheduler_group_id(scheduler_group_id);

        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS SCHEDULER_GROUP Create Unexpected error code");
        if (scheduler_group_id)
            p_switch->release_scheduler_group_id(scheduler_group_id);

        return NAS_QOS_E_FAIL;
    }

#endif
    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_scheduler_group_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{
    cps_api_object_attr_t sg_attr = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID);

    if (sg_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                     "switch id and scheduler group id not specified in the message\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint_t switch_id = 0;
    nas_obj_id_t scheduler_group_id = cps_api_object_attr_data_u64(sg_attr);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modify switch id %u, scheduler_group id %u\n",
                    switch_id, scheduler_group_id);

    nas_qos_scheduler_group * scheduler_group_p = nas_qos_cps_get_scheduler_group(switch_id,
                                                                                  scheduler_group_id);
    if (scheduler_group_p == NULL) {
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing scheduler_group */
    nas_qos_scheduler_group scheduler_group(*scheduler_group_p);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modify switch id %u, scheduler_group id %u . After copy\n",
                    switch_id, scheduler_group_id);

    cps_api_return_code_t rc = cps_api_ret_code_OK;
    if ((rc = nas_qos_cps_parse_attr(obj, scheduler_group)) != cps_api_ret_code_OK)
        return rc;

    if (scheduler_group.get_level() == 0 &&
        scheduler_group.dirty_attr_list().contains(BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID)) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                " Scheduler Group Level 0 node does not accept Scheduler Profile ID; "
                " Use Port Egress Object to configure Scheduler Profile!\n");
        return NAS_QOS_E_UNSUPPORTED;
    }

    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying scheduler_group %u attr on port %u \n",
                     scheduler_group.get_scheduler_group_id(), scheduler_group.get_port_id());

        nas::attr_set_t modified_attr_list = scheduler_group.commit_modify(*scheduler_group_p, (sav_obj? false: true));

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "done with commit_modify \n");


        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current scheduler_group value
        if (sav_obj) {
            cps_api_object_t tmp_obj;
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (!tmp_obj) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *scheduler_group_p);
       }

        // update the local cache with newly set values
        *scheduler_group_p = scheduler_group;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS SCHEDULER_GROUP Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS SCHEDULER_GROUP Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_api_scheduler_group_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{
    cps_api_object_attr_t sg_attr = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID);

    if (sg_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " scheduler group id not specified in the message\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint_t switch_id = 0;
    nas_obj_id_t scheduler_group_id = cps_api_object_attr_data_u64(sg_attr);

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " switch: %u not found\n",
                     switch_id);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_scheduler_group *scheduler_group_p = p_switch->get_scheduler_group(scheduler_group_id);
    if (scheduler_group_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " scheduler_group id: %u not found\n",
                     scheduler_group_id);

        return NAS_QOS_E_FAIL;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Deleting scheduler_group %u on switch: %u\n",
                 scheduler_group_p->get_scheduler_group_id(), p_switch->id());


    // delete
    try {
        scheduler_group_p->commit_delete(sav_obj? false: true);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Saving deleted scheduler_group %u\n",
                     scheduler_group_p->get_scheduler_group_id());

         // save current scheduler_group config for rollback if caller requests it.
        // use existing set_mask, existing config
        if (sav_obj) {
            cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (!tmp_obj) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, scheduler_group_p->set_attr_list(), *scheduler_group_p);
        }

        p_switch->remove_scheduler_group(scheduler_group_p->get_scheduler_group_id());

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS SCHEDULER_GROUP Delete error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;
    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS SCHEDULER_GROUP Delete: Unexpected error");
        return NAS_QOS_E_FAIL;
    }


    return cps_api_ret_code_OK;
}


/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_scheduler_group &scheduler_group)
{
    uint_t val;
    uint64_t val64;
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_SCHEDULER_GROUP_SWITCH_ID:
        case BASE_QOS_SCHEDULER_GROUP_ID:
            break; // These are for part of the keys

        case BASE_QOS_SCHEDULER_GROUP_CHILD_COUNT:
        case BASE_QOS_SCHEDULER_GROUP_CHILD_LIST:
            // non-configurable.
            break;


        case BASE_QOS_SCHEDULER_GROUP_PORT_ID:
            val = cps_api_object_attr_data_u32(it.attr);
            if (scheduler_group.set_port_id(val) != STD_ERR_OK)
                return NAS_QOS_E_FAIL;
            break;

        case BASE_QOS_SCHEDULER_GROUP_LEVEL:
            val = cps_api_object_attr_data_u32(it.attr);
            scheduler_group.set_level(val);
            break;

        case BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID:
            val64 = cps_api_object_attr_data_u64(it.attr);
            scheduler_group.set_scheduler_profile_id(val64);
            break;

        case CPS_API_ATTR_RESERVE_RANGE_END:
            // skip keys
            break;

       default:
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "Unrecognized option: %d", id);
            return NAS_QOS_E_UNSUPPORTED;
        }
    }


    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                    const nas::attr_set_t attr_set,
                                                    const nas_qos_scheduler_group &scheduler_group)
{
    // filling in the keys
    uint32_t switch_id = scheduler_group.switch_id();
    nas_obj_id_t sg_id = scheduler_group.get_scheduler_group_id();
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_GROUP_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_GROUP_SWITCH_ID,
            cps_api_object_ATTR_T_U32,
            &switch_id, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID,
            cps_api_object_ATTR_T_U64,
            &sg_id, sizeof(uint64_t));


    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_SCHEDULER_GROUP_ID:
            /* key */
            break;

        case BASE_QOS_SCHEDULER_GROUP_CHILD_COUNT:
        case BASE_QOS_SCHEDULER_GROUP_CHILD_LIST:
            /* Read only attributes, no need to save */
            break;

        case BASE_QOS_SCHEDULER_GROUP_PORT_ID:
            cps_api_object_attr_add_u32(obj, attr_id,
                    scheduler_group.get_level());
            break;

        case BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID:
            cps_api_object_attr_add_u64(obj, attr_id,
                    scheduler_group.get_scheduler_profile_id());
            break;

        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static nas_qos_scheduler_group * nas_qos_cps_get_scheduler_group(uint_t switch_id,
                                           nas_obj_id_t scheduler_group_id)
{

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NULL;

    nas_qos_scheduler_group *scheduler_group_p = p_switch->get_scheduler_group(scheduler_group_id);

    return scheduler_group_p;
}

