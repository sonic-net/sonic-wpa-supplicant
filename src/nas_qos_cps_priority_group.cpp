
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*!
 * \file   nas_qos_cps_priority_group.cpp
 * \brief  NAS qos priority_group related CPS API routines
 * \date   05-2016
 * \author
 */

#include "cps_api_events.h"
#include "cps_api_operation.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"

#include "event_log_types.h"
#include "event_log.h"
#include "std_error_codes.h"
#include "std_mutex_lock.h"

#include "nas_switch.h"
#include "hal_if_mapping.h"

#include "nas_qos_common.h"
#include "nas_qos_switch_list.h"
#include "nas_qos_cps.h"
#include "cps_api_key.h"
#include "dell-base-qos.h"
#include "nas_qos_priority_group.h"

#include <vector>

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                        nas_qos_priority_group &priority_group);
static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                        const nas::attr_set_t attr_set,
                                        const nas_qos_priority_group &priority_group);
static nas_qos_priority_group * nas_qos_cps_get_priority_group(uint_t switch_id,
                        nas_qos_priority_group_key_t key);
static cps_api_return_code_t nas_qos_cps_api_priority_group_set(
                                cps_api_object_t obj,
                                cps_api_object_t sav_obj);
static t_std_error nas_qos_port_priority_group_init(hal_ifindex_t ifindex);

static std_mutex_lock_create_static_init_rec(priority_group_mutex);

/**
  * This function provides NAS-QoS priority_group CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_priority_group_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&priority_group_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
    case cps_api_oper_DELETE:
        return NAS_QOS_E_FAIL; //not supported

    case cps_api_oper_SET:
        return nas_qos_cps_api_priority_group_set(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}

static cps_api_return_code_t nas_qos_cps_get_priority_group_info(
                                cps_api_get_params_t * param,
                                uint32_t switch_id, uint_t port_id,
                                bool match_local_id, uint_t local_id)
{
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    uint_t count = p_switch->get_number_of_port_priority_groups(port_id);

    if  (count == 0) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "switch id %u, port id %u has no priority_groups\n",
                switch_id, port_id);

        return NAS_QOS_E_FAIL;
    }

    std::vector<nas_qos_priority_group *> pg_list(count);
    p_switch->get_port_priority_groups(port_id, count, &pg_list[0]);

    /* fill in data */
    cps_api_object_t ret_obj;

    for (uint_t i = 0; i < count; i++ ) {
        nas_qos_priority_group *priority_group = pg_list[i];

        // filter out unwanted priority_groups

        if (match_local_id && (priority_group->get_local_id() != local_id))
            continue;


        ret_obj = cps_api_object_list_create_obj_and_append(param->list);
        if (ret_obj == NULL) {
            return cps_api_ret_code_ERR;
        }

        cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
                BASE_QOS_PRIORITY_GROUP_OBJ,
                cps_api_qualifier_TARGET);
        uint32_t val_port = priority_group->get_port_id();
        uint8_t  val_local_id = priority_group->get_local_id();

        cps_api_set_key_data(ret_obj, BASE_QOS_PRIORITY_GROUP_PORT_ID,
                cps_api_object_ATTR_T_U32,
                &val_port, sizeof(uint32_t));
        cps_api_set_key_data(ret_obj, BASE_QOS_PRIORITY_GROUP_LOCAL_ID,
                cps_api_object_ATTR_T_BIN,
                &val_local_id, sizeof(uint8_t));

        cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PRIORITY_GROUP_ID,
                priority_group->get_priority_group_id());

        // User configured objects
        if (priority_group->is_buffer_profile_set())
            cps_api_object_attr_add_u64(ret_obj,
                    BASE_QOS_PRIORITY_GROUP_BUFFER_PROFILE_ID,
                    priority_group->get_buffer_profile());

    }

    return cps_api_ret_code_OK;
}

/**
  * This function provides NAS-QoS priority_group CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_priority_group_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t port_id_attr = cps_api_get_key_data(obj, BASE_QOS_PRIORITY_GROUP_PORT_ID);
    cps_api_object_attr_t local_id_attr = cps_api_get_key_data(obj, BASE_QOS_PRIORITY_GROUP_LOCAL_ID);

    uint_t switch_id = 0;

    if (port_id_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Port Id must be specified\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint_t port_id = cps_api_object_attr_data_u32(port_id_attr);

    bool local_id_specified = false;
    uint8_t local_id;
    if (local_id_attr) {
        local_id = *(uint8_t *)cps_api_object_attr_data_bin(local_id_attr);
        local_id_specified = true;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, port_id id %u\n",
                    switch_id, port_id);

    std_mutex_simple_lock_guard p_m(&priority_group_mutex);

    // If the port priority group is not initialized yet, initialize it in NAS
    nas_qos_port_priority_group_init(port_id);

    return nas_qos_cps_get_priority_group_info(param, switch_id, port_id,
                                            local_id_specified, local_id);
}




/**
  * This function provides NAS-QoS priority_group CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_priority_group_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&priority_group_mutex);

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_priority_group_set(obj, NULL);
    }

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_api_priority_group_set(
                                cps_api_object_t obj,
                                cps_api_object_t sav_obj)
{
    cps_api_object_t tmp_obj;

    cps_api_object_attr_t port_id_attr = cps_api_get_key_data(obj, BASE_QOS_PRIORITY_GROUP_PORT_ID);
    cps_api_object_attr_t local_id_attr = cps_api_get_key_data(obj, BASE_QOS_PRIORITY_GROUP_LOCAL_ID);

    if (port_id_attr == NULL ||
        local_id_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Key incomplete in the message\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint32_t switch_id = 0;
    uint_t port_id = cps_api_object_attr_data_u32(port_id_attr);
    uint8_t local_id = *(uint8_t *)cps_api_object_attr_data_bin(local_id_attr);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
            "Modify switch id %u, port id %u,  local_id %u \n",
            switch_id, port_id,  local_id);

    // If the port is not be initialized yet, initialize it in NAS
    nas_qos_port_priority_group_init(port_id);

    nas_qos_priority_group_key_t key;
    key.port_id = port_id;
    key.local_id = local_id;
    nas_qos_priority_group * priority_group_p = nas_qos_cps_get_priority_group(switch_id, key);
    if (priority_group_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                        "priority_group not found in switch id %u\n",
                        switch_id);
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing priority_group */
    nas_qos_priority_group priority_group(*priority_group_p);

    cps_api_return_code_t rc = cps_api_ret_code_OK;
    if ((rc = nas_qos_cps_parse_attr(obj, priority_group)) != cps_api_ret_code_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Invalid information in the packet");
        return rc;
    }


    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "Modifying switch id %u, port id %u priority_group info \n",
                switch_id, port_id);

        nas::attr_set_t modified_attr_list = priority_group.commit_modify(
                                    *priority_group_p, (sav_obj? false: true));


        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current priority_group value
        if (sav_obj) {
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (!tmp_obj) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *priority_group_p);
        }

        // update the local cache with newly set values
        *priority_group_p = priority_group;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS priority_group Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS priority_group Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }


    return cps_api_ret_code_OK;
}

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_priority_group &priority_group)
{
    uint64_t val;
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_PRIORITY_GROUP_PORT_ID:
        case BASE_QOS_PRIORITY_GROUP_LOCAL_ID:
        case BASE_QOS_PRIORITY_GROUP_ID:
            break; // These are not settable from cps

        case BASE_QOS_PRIORITY_GROUP_BUFFER_PROFILE_ID:
            val = cps_api_object_attr_data_u64(it.attr);
            priority_group.mark_attr_dirty(id);
            priority_group.set_buffer_profile(val);
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
                                const nas_qos_priority_group &priority_group)
{
    // filling in the keys
    uint32_t val_port = priority_group.get_port_id();
    uint8_t local_id = priority_group.get_local_id();
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_PRIORITY_GROUP_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PRIORITY_GROUP_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &val_port, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_PRIORITY_GROUP_LOCAL_ID,
            cps_api_object_ATTR_T_BIN,
            &local_id, sizeof(uint8_t));


    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_PRIORITY_GROUP_PORT_ID:
        case BASE_QOS_PRIORITY_GROUP_LOCAL_ID:
        case BASE_QOS_PRIORITY_GROUP_ID:
            /* non-settable attr     */
            break;

        case BASE_QOS_PRIORITY_GROUP_BUFFER_PROFILE_ID:
            cps_api_object_attr_add_u64(obj, attr_id,
                    priority_group.get_buffer_profile());
            break;


        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static nas_qos_priority_group * nas_qos_cps_get_priority_group(uint_t switch_id,
                                           nas_qos_priority_group_key_t key)
{

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NULL;

    nas_qos_priority_group *priority_group_p = p_switch->get_priority_group(key);

    return priority_group_p;
}

// create per-port, per-priority_group instance
static t_std_error create_port_priority_group(hal_ifindex_t port_id,
                                    uint8_t local_id,
                                    ndi_obj_id_t ndi_priority_group_id)
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
        // create the priority_group and add the priority_group to switch
        nas_obj_id_t priority_group_id = switch_p->alloc_priority_group_id();
        nas_qos_priority_group_key_t key;
        key.port_id = intf_ctrl.if_index;
        key.local_id = local_id;
        nas_qos_priority_group pg (switch_p, key);

        pg.set_priority_group_id(priority_group_id);
        pg.add_npu(intf_ctrl.npu_id);
        pg.set_ndi_port_id(intf_ctrl.npu_id, intf_ctrl.port_id);
        pg.set_ndi_obj_id(ndi_priority_group_id);
        pg.mark_ndi_created();

        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "NAS priority_group_id 0x%016lX is allocated for priority_group:"
                     "local_qid %u, ndi_priority_group_id 0x%016lX",
                     priority_group_id, local_id, ndi_priority_group_id);
       switch_p->add_priority_group(pg);
    }
    catch (...) {
        return NAS_QOS_E_FAIL;
    }

    return STD_ERR_OK;

}


/* This function initializes the priority_groups of a port
 * @Return standard error code
 */
static t_std_error nas_qos_port_priority_group_init(hal_ifindex_t ifindex)
{
    interface_ctrl_t intf_ctrl;

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

    if (switch_p->port_priority_group_is_initialized(ifindex))
        return STD_ERR_OK; // initialized

    ndi_port_t ndi_port_id;
    ndi_port_id.npu_id = intf_ctrl.npu_id;
    ndi_port_id.npu_port = intf_ctrl.port_id;

    /* get the number of priority_groups per port */
    uint_t no_of_priority_group = ndi_qos_get_number_of_priority_groups(ndi_port_id);
    if (no_of_priority_group == 0) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "No priority_groups for npu_id %u, npu_port_id %u ",
                     ndi_port_id.npu_id, ndi_port_id.npu_port);
        return STD_ERR_OK;
    }

    /* get the list of ndi_priority_group id list */
    std::vector<ndi_obj_id_t> ndi_priority_group_id_list(no_of_priority_group);
    if (ndi_qos_get_priority_group_id_list(ndi_port_id, no_of_priority_group,
                                           &ndi_priority_group_id_list[0]) !=
            no_of_priority_group) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Fail to retrieve all priority_groups of npu_id %u, npu_port_id %u ",
                     ndi_port_id.npu_id, ndi_port_id.npu_port);
        return NAS_QOS_E_FAIL;
    }

    /* retrieve the ndi-priority_group type and
     * assign the priority_groups to with nas_priority_group_key
     */
    ndi_qos_priority_group_attribute_t ndi_qos_priority_group_attr;
    uint8_t local_id = 0;

    hal_ifindex_t port_id = ifindex;

    for (uint_t idx = 0; idx < no_of_priority_group; idx++) {

        ndi_qos_get_priority_group_attribute(ndi_port_id,
                                    ndi_priority_group_id_list[idx],
                                    &ndi_qos_priority_group_attr);

        local_id++;

        // Internally create NAS priority_group nodes and add to NAS QOS
        if (create_port_priority_group(port_id, local_id,
                    ndi_priority_group_id_list[idx]) != STD_ERR_OK) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                         "Not able to create ifindex %u, local_id %u",
                         port_id, local_id);
            return NAS_QOS_E_FAIL;
        }

    }

    return STD_ERR_OK;

}

static uint64_t get_stats_by_type(const nas_qos_priority_group_stat_counter_t *p,
                                BASE_QOS_PRIORITY_GROUP_STAT_t id)
{
    switch (id) {
    case BASE_QOS_PRIORITY_GROUP_STAT_PACKETS:
        return (p->packets);
    case BASE_QOS_PRIORITY_GROUP_STAT_BYTES:
        return (p->bytes);
    case BASE_QOS_PRIORITY_GROUP_STAT_CURRENT_OCCUPANCY_BYTES:
        return (p->current_occupancy_bytes);
    case BASE_QOS_PRIORITY_GROUP_STAT_WATERMARK_BYTES:
        return (p->watermark_bytes);

    default:
        return 0;
    }
}


/**
  * This function provides NAS-QoS priority_group stats CPS API read function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_priority_group_stat_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{

    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t port_id_attr = cps_api_get_key_data(obj, BASE_QOS_PRIORITY_GROUP_STAT_PORT_ID);
    cps_api_object_attr_t local_id_attr = cps_api_get_key_data(obj, BASE_QOS_PRIORITY_GROUP_STAT_LOCAL_ID);

    if (port_id_attr == NULL ||
        local_id_attr == NULL ) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "Incomplete key: port-id, priority_group local id must be specified\n");
        return NAS_QOS_E_MISSING_KEY;
    }
    uint32_t switch_id = 0;
    nas_qos_priority_group_key_t key;
    key.port_id = cps_api_object_attr_data_u32(port_id_attr);
    key.local_id = *(uint8_t *)cps_api_object_attr_data_bin(local_id_attr);


    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
            "Read switch id %u, port_id %u priority_group local id %u stat\n",
            switch_id, key.port_id, key.local_id);

    std_mutex_simple_lock_guard p_m(&priority_group_mutex);

    // If the port is not be initialized yet, initialize it in NAS
    nas_qos_port_priority_group_init(key.port_id);

    nas_qos_switch *switch_p = nas_qos_get_switch(switch_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "switch_id %u not found", switch_id);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_priority_group * priority_group_p = switch_p->get_priority_group(key);
    if (priority_group_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Priority Group not found");
        return NAS_QOS_E_FAIL;
    }

    nas_qos_priority_group_stat_counter_t stats = {0};
    std::vector<BASE_QOS_PRIORITY_GROUP_STAT_t> counter_ids;

    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_PRIORITY_GROUP_STAT_PORT_ID:
        case BASE_QOS_PRIORITY_GROUP_STAT_LOCAL_ID:
            break;

        case BASE_QOS_PRIORITY_GROUP_STAT_PACKETS:
        case BASE_QOS_PRIORITY_GROUP_STAT_BYTES:
        case BASE_QOS_PRIORITY_GROUP_STAT_CURRENT_OCCUPANCY_BYTES:
        case BASE_QOS_PRIORITY_GROUP_STAT_WATERMARK_BYTES:
            counter_ids.push_back((BASE_QOS_PRIORITY_GROUP_STAT_t)id);
            break;

        default:
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Unknown priority_group STAT flag: %u, ignored", id);
            break;
        }
    }

    if (ndi_qos_get_priority_group_stats(priority_group_p->get_ndi_port_id(),
                                priority_group_p->ndi_obj_id(),
                                &counter_ids[0],
                                counter_ids.size(),
                                &stats) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Priority Group stats get failed");
        return NAS_QOS_E_FAIL;
    }

    // return stats objects to cps-app
    cps_api_object_t ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL) {
        return cps_api_ret_code_ERR;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
            BASE_QOS_PRIORITY_GROUP_STAT_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_PRIORITY_GROUP_STAT_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &(key.port_id), sizeof(uint32_t));
    cps_api_set_key_data(ret_obj, BASE_QOS_PRIORITY_GROUP_STAT_LOCAL_ID,
            cps_api_object_ATTR_T_BIN,
            &(key.local_id), sizeof(uint8_t));

    uint64_t val64;
    for (uint_t i=0; i< counter_ids.size(); i++) {
        BASE_QOS_PRIORITY_GROUP_STAT_t id = counter_ids[i];
        val64 = get_stats_by_type(&stats, id);
        cps_api_object_attr_add_u64(ret_obj, id, val64);
    }

    return  cps_api_ret_code_OK;

}


/**
  * This function provides NAS-QoS priority_group stats CPS API clear function
  * To clear the priority_group stats, set relevant counters to zero
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_priority_group_stat_clear (void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    if (op != cps_api_oper_SET)
        return NAS_QOS_E_UNSUPPORTED;

    cps_api_object_attr_t port_id_attr = cps_api_get_key_data(obj, BASE_QOS_PRIORITY_GROUP_STAT_PORT_ID);
    cps_api_object_attr_t local_id_attr = cps_api_get_key_data(obj, BASE_QOS_PRIORITY_GROUP_STAT_LOCAL_ID);

    if (port_id_attr == NULL ||
        local_id_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "Incomplete key: port-id, priority_group local id must be specified\n");
        return NAS_QOS_E_MISSING_KEY;
    }
    uint32_t switch_id = 0;
    nas_qos_priority_group_key_t key;
    key.port_id = cps_api_object_attr_data_u32(port_id_attr);
    key.local_id = *(uint8_t *)cps_api_object_attr_data_bin(local_id_attr);


    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
            "Read switch id %u, port_id %u priority_group local_id %u stat\n",
            switch_id, key.port_id,  key.local_id);

    std_mutex_simple_lock_guard p_m(&priority_group_mutex);

    // If the port is not be initialized yet, initialize it in NAS
    nas_qos_port_priority_group_init(key.port_id);

    nas_qos_switch *switch_p = nas_qos_get_switch(switch_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "switch_id %u not found", switch_id);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_priority_group * priority_group_p = switch_p->get_priority_group(key);
    if (priority_group_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Priority Group not found");
        return NAS_QOS_E_FAIL;
    }

    std::vector<BASE_QOS_PRIORITY_GROUP_STAT_t> counter_ids;

    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_PRIORITY_GROUP_STAT_PORT_ID:
        case BASE_QOS_PRIORITY_GROUP_STAT_LOCAL_ID:
            break;

        case BASE_QOS_PRIORITY_GROUP_STAT_PACKETS:
        case BASE_QOS_PRIORITY_GROUP_STAT_BYTES:
        case BASE_QOS_PRIORITY_GROUP_STAT_WATERMARK_BYTES:
            counter_ids.push_back((BASE_QOS_PRIORITY_GROUP_STAT_t)id);
            break;

        case BASE_QOS_PRIORITY_GROUP_STAT_CURRENT_OCCUPANCY_BYTES:
            // READ-only
            break;

        default:
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                    "Unknown priority_group STAT flag: %u, ignored", id);
            break;
        }
    }

    if (ndi_qos_clear_priority_group_stats(priority_group_p->get_ndi_port_id(),
                                priority_group_p->ndi_obj_id(),
                                &counter_ids[0],
                                counter_ids.size()) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Priority Group stats clear failed");
        return NAS_QOS_E_FAIL;
    }


    return  cps_api_ret_code_OK;

}
