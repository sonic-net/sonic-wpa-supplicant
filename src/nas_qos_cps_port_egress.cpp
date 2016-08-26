
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


#include "cps_api_events.h"
#include "cps_api_operation.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"

#include "event_log_types.h"
#include "event_log.h"
#include "std_error_codes.h"
#include "std_mutex_lock.h"

#include "hal_if_mapping.h"
#include "nas_qos_common.h"
#include "nas_qos_switch_list.h"
#include "nas_qos_cps.h"
#include "cps_api_key.h"
#include "dell-base-qos.h"
#include "nas_qos_port_egress.h"

static std_mutex_lock_create_static_init_rec(port_eg_mutex);

static cps_api_return_code_t nas_qos_cps_parse_attr(cps_api_object_t obj,
                                                nas_qos_port_egress& port_eg)
{
    uint64_t lval;
    uint8_t *pval;
    cps_api_object_it_t it;

    cps_api_object_it_begin(obj, &it);
    for ( ; cps_api_object_it_valid(&it); cps_api_object_it_next(&it)) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch(id) {
        case BASE_QOS_PORT_EGRESS_SWITCH_ID:
        case BASE_QOS_PORT_EGRESS_PORT_ID:
            break;
        case BASE_QOS_PORT_EGRESS_BUFFER_LIMIT:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_buffer_limit(lval);
            break;
        case BASE_QOS_PORT_EGRESS_WRED_PROFILE_ID:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_wred_profile_id(lval);
            break;
        case BASE_QOS_PORT_EGRESS_SCHEDULER_PROFILE_ID:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_scheduler_profile_id(lval);
            break;
        case BASE_QOS_PORT_EGRESS_NUM_UNICAST_QUEUE:
            pval = (uint8_t *)cps_api_object_attr_data_bin(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_num_unicast_queue(*pval);
            break;
        case BASE_QOS_PORT_EGRESS_NUM_MULTICAST_QUEUE:
            pval = (uint8_t *)cps_api_object_attr_data_bin(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_num_multicast_queue(*pval);
            break;
        case BASE_QOS_PORT_EGRESS_NUM_QUEUE:
            pval = (uint8_t *)cps_api_object_attr_data_bin(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_num_queue(*pval);
            break;
        case BASE_QOS_PORT_EGRESS_QUEUE_ID_LIST:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.add_queue_id(lval);
            break;
        case BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_tc_to_queue_map(lval);
            break;
        case BASE_QOS_PORT_EGRESS_TC_TO_DOT1P_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_tc_to_dot1p_map(lval);
            break;
        case BASE_QOS_PORT_EGRESS_TC_TO_DSCP_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_tc_to_dscp_map(lval);
            break;
        case BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_tc_color_to_dot1p_map(lval);
            break;
        case BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_tc_color_to_dscp_map(lval);
            break;
        case BASE_QOS_PORT_EGRESS_PFC_PRIORITY_TO_QUEUE_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_eg.mark_attr_dirty(id);
            port_eg.set_pfc_priority_to_queue_map(lval);
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

static nas_qos_port_egress* nas_qos_get_port_egress(uint_t switch_id,
                                                hal_ifindex_t port_id)
{
    nas_qos_switch* p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_WARNING, "NAS-QOS",
                "Switch not found, id=%d\n", switch_id);
        return NULL;
    }
    return p_switch->get_port_egress(port_id);
}

static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                    const nas::attr_set_t& attr_set,
                                                    const nas_qos_port_egress& port_eg)
{
    // filling in the keys
    hal_ifindex_t port_id = port_eg.get_port_id();
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_PORT_EGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_EGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &port_id, sizeof(uint32_t));


    uint8_t bval;
    uint32_t idx = 0;

    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_PORT_EGRESS_PORT_ID:
            /* key */
            break;
        case BASE_QOS_PORT_EGRESS_BUFFER_LIMIT:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_buffer_limit());
            break;
        case BASE_QOS_PORT_EGRESS_WRED_PROFILE_ID:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_wred_profile_id());
            break;
        case BASE_QOS_PORT_EGRESS_SCHEDULER_PROFILE_ID:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_scheduler_profile_id());
            break;
        case BASE_QOS_PORT_EGRESS_NUM_UNICAST_QUEUE:
            bval = port_eg.get_num_unicast_queue();
            cps_api_object_attr_add(obj, attr_id, &bval, 1);
            break;
        case BASE_QOS_PORT_EGRESS_NUM_MULTICAST_QUEUE:
            bval = port_eg.get_num_multicast_queue();
            cps_api_object_attr_add(obj, attr_id, &bval, 1);
            break;
        case BASE_QOS_PORT_EGRESS_NUM_QUEUE:
            bval = port_eg.get_num_queue();
            cps_api_object_attr_add(obj, attr_id, &bval, 1);
            break;
        case BASE_QOS_PORT_EGRESS_QUEUE_ID_LIST:
            if (idx >= port_eg.get_queue_id_count()) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_WARNING, "NAS-QOS",
                             "queue_id %d not less than total count %d\n",
                             idx, port_eg.get_queue_id_count());
                break;
            }
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_queue_id(idx));
            idx ++;
            break;
        case BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_tc_to_queue_map());
            break;
        case BASE_QOS_PORT_EGRESS_TC_TO_DOT1P_MAP:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_tc_to_dot1p_map());
            break;
        case BASE_QOS_PORT_EGRESS_TC_TO_DSCP_MAP:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_tc_to_dscp_map());
            break;
        case BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_tc_color_to_dot1p_map());
            break;
        case BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_tc_color_to_dscp_map());
            break;
        case BASE_QOS_PORT_EGRESS_PFC_PRIORITY_TO_QUEUE_MAP:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_eg.get_pfc_priority_to_queue_map());
            break;
        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static t_std_error create_port_egr_profile(uint_t switch_id,
                                         hal_ifindex_t port_id)
{
    static const int MAX_QUEUE_ID_NUM = 128;
    ndi_obj_id_t queue_id_list[MAX_QUEUE_ID_NUM] = {0};
    interface_ctrl_t intf_ctrl;
    BASE_QOS_PORT_EGRESS_t attr_list[] = {
        BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP,
        BASE_QOS_PORT_EGRESS_TC_TO_DOT1P_MAP,
        BASE_QOS_PORT_EGRESS_TC_TO_DSCP_MAP,
        BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP,
        BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP,
        BASE_QOS_PORT_EGRESS_WRED_PROFILE_ID,
        BASE_QOS_PORT_EGRESS_SCHEDULER_PROFILE_ID,
        BASE_QOS_PORT_EGRESS_QUEUE_ID_LIST,
        BASE_QOS_PORT_EGRESS_PFC_PRIORITY_TO_QUEUE_MAP,
    };
    qos_port_egr_struct_t p;

    intf_ctrl.q_type = HAL_INTF_INFO_FROM_IF;
    intf_ctrl.if_index = port_id;
    intf_ctrl.vrf_id = 0;
    if (dn_hal_get_interface_info(&intf_ctrl) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "Failed to read interface info, if_index=%u\n", port_id);
        return NAS_QOS_E_FAIL;
    }

    memset(&p, 0, sizeof(qos_port_egr_struct_t));
    p.queue_id_list = queue_id_list;
    p.num_queue_id = MAX_QUEUE_ID_NUM;
    int attr_num = sizeof(attr_list)/sizeof(attr_list[0]);
    for (int idx = 0; idx < attr_num; idx ++) {
        int rc = ndi_qos_get_port_egr_profile(intf_ctrl.npu_id,
                                        intf_ctrl.port_id,
                                        &attr_list[idx], 1,
                                        &p);
        if (rc != STD_ERR_OK) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                    "Attribute %u is not supported by NDI for reading\n", attr_list[idx]);
            if (attr_list[idx] ==  BASE_QOS_PORT_EGRESS_QUEUE_ID_LIST) {
                p.num_queue_id = 0;
            }
        }
    }

    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_WARNING, "QOS",
            "Create port egress profile: switch %d port %d\n",
            switch_id, port_id);
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        return NAS_QOS_E_FAIL;
    }

    try {
        nas_qos_port_egress port_eg(p_switch, port_id);
        port_eg.set_buffer_limit(p.buffer_limit);
        port_eg.set_wred_profile_id(p.wred_profile_id);
        port_eg.set_num_unicast_queue(p.num_ucast_queue);
        port_eg.set_num_multicast_queue(p.num_mcast_queue);
        port_eg.set_num_queue(p.num_queue);
        for (uint_t idx = 0; idx < p.num_queue_id; idx ++) {
            port_eg.add_queue_id(p.queue_id_list[idx]);
        }
        port_eg.set_tc_to_queue_map(p.tc_to_queue_map);
        port_eg.set_tc_to_dot1p_map(p.tc_to_dot1p_map);
        port_eg.set_tc_to_dscp_map(p.tc_to_dscp_map);
        port_eg.set_tc_color_to_dot1p_map(p.tc_to_dot1p_map);
        port_eg.set_tc_color_to_dscp_map(p.tc_to_dscp_map);
        port_eg.set_scheduler_profile_id(p.scheduler_profile_id);
        port_eg.set_pfc_priority_to_queue_map(p.pfc_priority_to_queue_map);

        port_eg.add_npu(intf_ctrl.npu_id);
        port_eg.set_ndi_port_id(intf_ctrl.npu_id, intf_ctrl.port_id);
        port_eg.mark_ndi_created();

        p_switch->add_port_egress(port_eg);
    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                "Exception on creating nas_qos_port_egress object");
        return NAS_QOS_E_FAIL;
    }

    return STD_ERR_OK;
}

/* This function pre-loads the NAS QoS module with default QoS Port-egress info
 * @Return standard error code
 */
t_std_error nas_qos_port_egress_init(uint_t switch_id,
                                    hal_ifindex_t ifindex)
{
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Failed to get switch, switch_id=%d\n",
                   switch_id);
        return NAS_QOS_E_FAIL;
    }

    if (p_switch->port_egr_is_initialized(ifindex)) {
        //already initialized
        return STD_ERR_OK;
    }

    if (create_port_egr_profile(switch_id, ifindex) !=
            STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                "Failed to create port egress profle: %u\n", ifindex);
        return NAS_QOS_E_FAIL;
    }

    dump_nas_qos_port_egress(switch_id);

    return STD_ERR_OK;
}

static cps_api_return_code_t get_cps_obj_switch_port_id(cps_api_object_t obj,
                                                uint_t& switch_id, uint_t& port_id)
{
    cps_api_object_attr_t attr;

    switch_id = 0;

    attr = cps_api_get_key_data(obj, BASE_QOS_PORT_EGRESS_PORT_ID);
    if (attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "port id not specified in message\n");
        return NAS_QOS_E_MISSING_KEY;
    }
    port_id = cps_api_object_attr_data_u32(attr);

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_port_eg_set(
                                            cps_api_object_t obj,
                                            cps_api_object_list_t sav_obj)
{
    uint_t switch_id = 0, port_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    if ((rc = get_cps_obj_switch_port_id(obj, switch_id, port_id)) !=
            cps_api_ret_code_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "Failed to get switch and port id");
        return rc;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modify switch id %u, port id %u\n",
                    switch_id, port_id);

    nas_qos_port_egress_init(switch_id, port_id);

    nas_qos_port_egress* port_eg_p = nas_qos_get_port_egress(switch_id, port_id);
    if (port_eg_p == NULL) {
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing port egress */
    nas_qos_port_egress port_eg(*port_eg_p);
    port_eg.clear_queue_id();

    if ((rc = nas_qos_cps_parse_attr(obj, port_eg)) != cps_api_ret_code_OK) {
        return rc;
    }

    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying port egress %u attr \n",
                     port_eg.get_port_id());

        nas::attr_set_t modified_attr_list = port_eg.commit_modify(*port_eg_p,
                                                    (sav_obj? false: true));

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "done with commit_modify \n");


        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current port egress value
        if (sav_obj) {
            cps_api_object_t tmp_obj;
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }

            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *port_eg_p);

       }

        // update the local cache with newly set values
        *port_eg_p = port_eg;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS PORT EGRESS Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS PORT EGRESS Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}

/*
  * This function provides NAS-QoS PORT-EGRESS CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_egress_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&port_eg_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
    case cps_api_oper_DELETE:
        // Port egress requires no creation or deletion
        return NAS_QOS_E_FAIL;

    case cps_api_oper_SET:
        return nas_qos_cps_api_port_eg_set(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}

/*
  * This function provides NAS-QoS PORT-EGRESS CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_egress_read(void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj;
    uint_t switch_id = 0, port_id = 0;
    uint8_t bval;
    uint32_t idx, queue_id_count;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    obj = cps_api_object_list_get(param->filters, ix);
    if (obj == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Object not exist\n");
        return cps_api_ret_code_ERR;
    }
    if ((rc = get_cps_obj_switch_port_id(obj, switch_id, port_id)) !=
            cps_api_ret_code_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "Failed to get switch and port id");
        return rc;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, port id %u\n",
                 switch_id, port_id);

    std_mutex_simple_lock_guard p_m(&port_eg_mutex);

    nas_qos_port_egress_init(switch_id, port_id);

    nas_qos_port_egress *port_eg = nas_qos_get_port_egress(switch_id, port_id);
    if (port_eg == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Failed to get port egress object, port_id=%d\n",
                   port_id);
        return NAS_QOS_E_FAIL;
    }

    /* fill in data */
    cps_api_object_t ret_obj;

    ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL){
        return cps_api_ret_code_ERR;
    }

    // @todo: handle getting individual attributes.
    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
            BASE_QOS_PORT_EGRESS_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_PORT_EGRESS_SWITCH_ID,
            cps_api_object_ATTR_T_U32,
            &switch_id, sizeof(uint32_t));
    cps_api_set_key_data(ret_obj, BASE_QOS_PORT_EGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &port_id, sizeof(uint32_t));
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_BUFFER_LIMIT,
                                port_eg->get_buffer_limit());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_WRED_PROFILE_ID,
                                port_eg->get_wred_profile_id());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_SCHEDULER_PROFILE_ID,
                                port_eg->get_scheduler_profile_id());
    bval = port_eg->get_num_unicast_queue();
    cps_api_object_attr_add(ret_obj, BASE_QOS_PORT_EGRESS_NUM_UNICAST_QUEUE, &bval, 1);
    bval = port_eg->get_num_multicast_queue();
    cps_api_object_attr_add(ret_obj, BASE_QOS_PORT_EGRESS_NUM_MULTICAST_QUEUE, &bval, 1);
    bval = port_eg->get_num_queue();
    cps_api_object_attr_add(ret_obj, BASE_QOS_PORT_EGRESS_NUM_QUEUE, &bval, 1);
    queue_id_count = port_eg->get_queue_id_count();
    for (idx = 0; idx < queue_id_count; idx ++) {
        cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_QUEUE_ID_LIST,
                                    port_eg->get_queue_id(idx));
    }
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP,
                                port_eg->get_tc_to_queue_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_TC_TO_DOT1P_MAP,
                                port_eg->get_tc_to_dot1p_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_TC_TO_DSCP_MAP,
                                port_eg->get_tc_to_dscp_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP,
                                port_eg->get_tc_color_to_dot1p_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP,
                                port_eg->get_tc_color_to_dscp_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_EGRESS_PFC_PRIORITY_TO_QUEUE_MAP,
                                port_eg->get_pfc_priority_to_queue_map());

    return cps_api_ret_code_OK;
}

/*
  * This function provides NAS-QoS PORT-EGRESS CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_egress_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    if (obj == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Object not exist\n");
        return cps_api_ret_code_ERR;
    }
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&port_eg_mutex);

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_port_eg_set(obj, NULL);
    }

    // create/delete are not allowed for queue, no roll-back is needed

    return cps_api_ret_code_OK;
}

/* Debugging and unit testing */
void dump_nas_qos_port_egress(nas_switch_id_t switch_id)
{
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);

    if (p_switch) {
        p_switch->dump_all_port_egr_profile();
    }
}
