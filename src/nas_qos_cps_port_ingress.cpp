
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
#include "nas_qos_port_ingress.h"

static std_mutex_lock_create_static_init_rec(port_ing_mutex);

static cps_api_return_code_t nas_qos_cps_parse_attr(cps_api_object_t obj,
                                                nas_qos_port_ingress& port_ing)
{
    uint_t val;
    uint64_t lval;
    cps_api_object_it_t it;

    cps_api_object_it_begin(obj, &it);
    for ( ; cps_api_object_it_valid(&it); cps_api_object_it_next(&it)) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch(id) {
        case BASE_QOS_PORT_INGRESS_SWITCH_ID:
        case BASE_QOS_PORT_INGRESS_PORT_ID:
            break;
        case BASE_QOS_PORT_INGRESS_DEFAULT_TRAFFIC_CLASS:
            val = cps_api_object_attr_data_u32(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_default_traffic_class(val);
            break;
        case BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_dot1p_to_tc_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_DOT1P_TO_COLOR_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_dot1p_to_color_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_dot1p_to_tc_color_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_DSCP_TO_TC_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_dscp_to_tc_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_DSCP_TO_COLOR_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_dscp_to_color_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_dscp_to_tc_color_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_tc_to_queue_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_FLOW_CONTROL:
            val = cps_api_object_attr_data_u32(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_flow_control(val);
            break;
        case BASE_QOS_PORT_INGRESS_POLICER_ID:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_policer_id(lval);
            break;
        case BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_flood_storm_control(lval);
            break;
        case BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_broadcast_storm_control(lval);
            break;
        case BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_multicast_storm_control(lval);
            break;
        case BASE_QOS_PORT_INGRESS_TC_TO_PRIORITY_GROUP_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_tc_to_priority_group_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP:
            lval = cps_api_object_attr_data_u64(it.attr);
            port_ing.mark_attr_dirty(id);
            port_ing.set_priority_group_to_pfc_priority_map(lval);
            break;
        case BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_NUMBER:
        case BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_ID_LIST:
            break; //non-settable
        case BASE_QOS_PORT_INGRESS_PER_PRIORITY_FLOW_CONTROL:
            {
                uint8_t val8 = *(uint8_t *)cps_api_object_attr_data_bin(it.attr);
                port_ing.mark_attr_dirty(id);
                port_ing.set_per_priority_flow_control(val8);
            }
            break;
        case CPS_API_ATTR_RESERVE_RANGE_END:
            // skip keys
            break;

        default:
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "NAS-QOS",
                    "Unrecognized option: %d", id);
            return NAS_QOS_E_UNSUPPORTED;
        }
    }

    return cps_api_ret_code_OK;
}

static nas_qos_port_ingress* nas_qos_get_port_ingress(uint_t switch_id,
                                                hal_ifindex_t port_id)
{
    nas_qos_switch* p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_WARNING, "NAS-QOS",
                "Switch not found, id=%d\n", switch_id);
        return NULL;
    }
    return p_switch->get_port_ingress(port_id);
}


static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                    const nas::attr_set_t& attr_set,
                                                    const nas_qos_port_ingress& port_ing)
{
    // filling in the keys
    hal_ifindex_t port_id = port_ing.get_port_id();
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_PORT_INGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_INGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &port_id, sizeof(uint32_t));


    uint8_t bit_vec;

    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_PORT_INGRESS_PORT_ID:
            /* key  */
            break;
        case BASE_QOS_PORT_INGRESS_DEFAULT_TRAFFIC_CLASS:
            cps_api_object_attr_add_u32(obj, BASE_QOS_PORT_INGRESS_DEFAULT_TRAFFIC_CLASS,
                                    port_ing.get_default_traffic_class());
            break;
        case BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_MAP:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_MAP,
                                    port_ing.get_dot1p_to_tc_map());
            break;
        case BASE_QOS_PORT_INGRESS_DOT1P_TO_COLOR_MAP:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_COLOR_MAP,
                                    port_ing.get_dot1p_to_color_map());
            break;
        case BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP,
                                    port_ing.get_dot1p_to_tc_color_map());
            break;
        case BASE_QOS_PORT_INGRESS_DSCP_TO_TC_MAP:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DSCP_TO_TC_MAP,
                                    port_ing.get_dscp_to_tc_map());
            break;
        case BASE_QOS_PORT_INGRESS_DSCP_TO_COLOR_MAP:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DSCP_TO_COLOR_MAP,
                                    port_ing.get_dscp_to_color_map());
            break;
        case BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP,
                                    port_ing.get_dscp_to_tc_color_map());
            break;
        case BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP,
                                    port_ing.get_tc_to_queue_map());
            break;
        case BASE_QOS_PORT_INGRESS_FLOW_CONTROL:
            cps_api_object_attr_add_u32(obj, BASE_QOS_PORT_INGRESS_FLOW_CONTROL,
                                    (uint32_t)port_ing.get_flow_control());
            break;
        case BASE_QOS_PORT_INGRESS_POLICER_ID:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_POLICER_ID,
                                    port_ing.get_policer_id());
            break;
        case BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL,
                                    port_ing.get_flood_storm_control());
            break;
        case BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL,
                                    port_ing.get_broadcast_storm_control());
            break;
        case BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL:
            cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL,
                                    port_ing.get_multicast_storm_control());
            break;
        case BASE_QOS_PORT_INGRESS_TC_TO_PRIORITY_GROUP_MAP:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_ing.get_tc_to_priority_group_map());
            break;
        case BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP:
            cps_api_object_attr_add_u64(obj, attr_id,
                                    port_ing.get_priority_group_to_pfc_priority_map());
            break;
        case BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_NUMBER:
        case BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_ID_LIST:
            break; //non-settable
        case BASE_QOS_PORT_INGRESS_PER_PRIORITY_FLOW_CONTROL:
            bit_vec = port_ing.get_per_priority_flow_control();
            cps_api_object_attr_add(obj, attr_id,
                                    &bit_vec, sizeof(uint8_t));
            break;
        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static t_std_error create_port_ing_profile(uint_t switch_id,
                                         hal_ifindex_t port_id)
{
    static const int MAX_PRIORITY_GROUP_ID_NUM = 32;
    ndi_obj_id_t pg_id_list[MAX_PRIORITY_GROUP_ID_NUM] = {0};
    interface_ctrl_t intf_ctrl;
    BASE_QOS_PORT_INGRESS_t attr_list[] = {
        BASE_QOS_PORT_INGRESS_DEFAULT_TRAFFIC_CLASS,
        BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_MAP,
        BASE_QOS_PORT_INGRESS_DOT1P_TO_COLOR_MAP,
        BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP,
        BASE_QOS_PORT_INGRESS_DSCP_TO_TC_MAP,
        BASE_QOS_PORT_INGRESS_DSCP_TO_COLOR_MAP,
        BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP,
        BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP,
        BASE_QOS_PORT_INGRESS_FLOW_CONTROL,
        BASE_QOS_PORT_INGRESS_POLICER_ID,
        BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL,
        BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL,
        BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL,
        BASE_QOS_PORT_INGRESS_TC_TO_PRIORITY_GROUP_MAP,
        BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP,
        BASE_QOS_PORT_INGRESS_PER_PRIORITY_FLOW_CONTROL,
        BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_NUMBER,
        BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_ID_LIST,
    };
    qos_port_ing_struct_t p;

    intf_ctrl.q_type = HAL_INTF_INFO_FROM_IF;
    intf_ctrl.if_index = port_id;
    intf_ctrl.vrf_id = 0;
    if (dn_hal_get_interface_info(&intf_ctrl) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "Failed to read interface info, if_index=%u\n", port_id);
        return NAS_QOS_E_FAIL;
    }

    memset(&p, 0, sizeof(qos_port_ing_struct_t));
    p.priority_group_id_list = pg_id_list;
    p.num_priority_group_id = MAX_PRIORITY_GROUP_ID_NUM;
    int attr_num = sizeof(attr_list)/sizeof(attr_list[0]);
    for (int idx = 0; idx < attr_num; idx ++) {
        int rc = ndi_qos_get_port_ing_profile(intf_ctrl.npu_id,
                                            intf_ctrl.port_id,
                                            &attr_list[idx], 1,
                                            &p);
        if (rc != STD_ERR_OK) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                    "Attribute %u is not supported by NDI for reading\n", attr_list[idx]);
        }
    }

    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_WARNING, "NAS-QOS",
            "Create port ingress profile: switch %d port %d\n",
            switch_id, port_id);
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        return NAS_QOS_E_FAIL;
    }

    try {
        nas_qos_port_ingress port_ing(p_switch, port_id);
        port_ing.set_default_traffic_class(p.default_tc);
        port_ing.set_dot1p_to_tc_map(p.dot1p_to_tc_map);
        port_ing.set_dot1p_to_color_map(p.dot1p_to_color_map);
        port_ing.set_dot1p_to_tc_color_map(p.dot1p_to_tc_color_map);
        port_ing.set_dscp_to_tc_map(p.dscp_to_tc_map);
        port_ing.set_dscp_to_color_map(p.dscp_to_color_map);
        port_ing.set_dscp_to_tc_color_map(p.dscp_to_tc_color_map);
        port_ing.set_tc_to_queue_map(p.tc_to_queue_map);
        port_ing.set_flow_control(p.flow_control);
        port_ing.set_policer_id(p.policer_id);
        port_ing.set_flood_storm_control(p.flood_storm_control);
        port_ing.set_broadcast_storm_control(p.bcast_storm_control);
        port_ing.set_multicast_storm_control(p.mcast_storm_control);
        port_ing.set_tc_to_priority_group_map(p.tc_to_priority_group_map);
        port_ing.set_priority_group_to_pfc_priority_map(p.priority_group_to_pfc_priority_map);
        port_ing.set_per_priority_flow_control(p.per_priority_flow_control);
        port_ing.set_priority_group_number(p.priority_group_number);
        for (uint_t idx = 0; idx < p.num_priority_group_id; idx ++) {
            port_ing.add_priority_group_id(p.priority_group_id_list[idx]);
        }

        port_ing.add_npu(intf_ctrl.npu_id);
        port_ing.set_ndi_port_id(intf_ctrl.npu_id, intf_ctrl.port_id);
        port_ing.mark_ndi_created();

        p_switch->add_port_ingress(port_ing);
    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "Exception on creating nas_qos_port_ingress object");
        return NAS_QOS_E_FAIL;
    }

    return STD_ERR_OK;
}

/* This function pre-loads the NAS QoS module with default QoS Port-ingress info
 * @Return standard error code
 */
t_std_error nas_qos_port_ingress_init(uint_t switch_id,
                                    hal_ifindex_t ifindex)
{
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Failed to get switch, switch_id=%d\n",
                   switch_id);
        return NAS_QOS_E_FAIL;
    }

    if (p_switch->port_ing_is_initialized(ifindex)) {
        //already initialized
        return STD_ERR_OK;
    }

    cps_api_return_code_t rc = cps_api_ret_code_OK;
    if ((rc = create_port_ing_profile(switch_id, ifindex)) !=
            STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "NAS-QOS",
                "Failed to create port ingress profle: %u\n", ifindex);
        return rc;
    }

    dump_nas_qos_port_ingress(switch_id);

    return STD_ERR_OK;
}

static cps_api_return_code_t get_cps_obj_switch_port_id(cps_api_object_t obj,
                                                uint_t& switch_id, uint_t& port_id)
{
    cps_api_object_attr_t attr;

    switch_id = 0;

    attr = cps_api_get_key_data(obj, BASE_QOS_PORT_INGRESS_PORT_ID);
    if (attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "port id not specified in message\n");
        return NAS_QOS_E_MISSING_KEY;
    }
    port_id = cps_api_object_attr_data_u32(attr);

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_port_ing_set(
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

    nas_qos_port_ingress_init(switch_id, port_id);

    nas_qos_port_ingress* port_ing_p = nas_qos_get_port_ingress(switch_id, port_id);
    if (port_ing_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "Could not find port ingress object for switch %d port %d\n",
                switch_id, port_id);
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing port ingress */
    nas_qos_port_ingress port_ing(*port_ing_p);

    if ((rc = nas_qos_cps_parse_attr(obj, port_ing)) != cps_api_ret_code_OK) {
        return rc;
    }

    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying port ingress %u attr \n",
                     port_ing.get_port_id());

        nas::attr_set_t modified_attr_list = port_ing.commit_modify(*port_ing_p,
                                                    (sav_obj? false: true));

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "done with commit_modify \n");


        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current port ingress value
        if (sav_obj) {
            cps_api_object_t tmp_obj;
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }

            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *port_ing_p);

       }

        // update the local cache with newly set values
        *port_ing_p = port_ing;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "NAS-QOS",
                    "NAS PORT INGRESS Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "NAS-QOS",
                    "NAS PORT INGRESS Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}

/*
  * This function provides NAS-QoS PORT-INGRESS CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_ingress_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    if (obj == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Object not exist\n");
        return cps_api_ret_code_ERR;
    }
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&port_ing_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
    case cps_api_oper_DELETE:
        // Port ingress requires no creation or deletion
        return NAS_QOS_E_FAIL;
    case cps_api_oper_SET:
        return nas_qos_cps_api_port_ing_set(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}

/*
  * This function provides NAS-QoS PORT-INGRESS CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_ingress_read(void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj;
    uint_t switch_id = 0, port_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    obj = cps_api_object_list_get(param->filters, ix);
    if (obj == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Object not exist\n");
        return NAS_QOS_E_FAIL;
    }
    if ((rc = get_cps_obj_switch_port_id(obj, switch_id, port_id)) !=
            cps_api_ret_code_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "NAS-QOS",
                "Failed to get switch and port id");
        return rc;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, port id %u\n",
                 switch_id, port_id);

    std_mutex_simple_lock_guard p_m(&port_ing_mutex);

    nas_qos_port_ingress_init(switch_id, port_id);

    nas_qos_port_ingress *port_ing = nas_qos_get_port_ingress(switch_id, port_id);
    if (port_ing == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Failed to get port ingress object, port_id=%d\n",
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
            BASE_QOS_PORT_INGRESS_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_PORT_INGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &port_id, sizeof(uint32_t));
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_PORT_INGRESS_DEFAULT_TRAFFIC_CLASS,
                                port_ing->get_default_traffic_class());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_MAP,
                                port_ing->get_dot1p_to_tc_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_COLOR_MAP,
                                port_ing->get_dot1p_to_color_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP,
                                port_ing->get_dot1p_to_tc_color_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_DSCP_TO_TC_MAP,
                                port_ing->get_dscp_to_tc_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_DSCP_TO_COLOR_MAP,
                                port_ing->get_dscp_to_color_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP,
                                port_ing->get_dscp_to_tc_color_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP,
                                port_ing->get_tc_to_queue_map());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_PORT_INGRESS_FLOW_CONTROL,
                                port_ing->get_flow_control());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_POLICER_ID,
                                port_ing->get_policer_id());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL,
                                port_ing->get_flood_storm_control());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL,
                                port_ing->get_broadcast_storm_control());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL,
                                port_ing->get_multicast_storm_control());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_TC_TO_PRIORITY_GROUP_MAP,
                                port_ing->get_tc_to_priority_group_map());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP,
                                port_ing->get_priority_group_to_pfc_priority_map());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_NUMBER,
                                port_ing->get_priority_group_number());
    uint8_t bit_vec = port_ing->get_per_priority_flow_control();
    cps_api_object_attr_add(ret_obj, BASE_QOS_PORT_INGRESS_PER_PRIORITY_FLOW_CONTROL,
                            &bit_vec, sizeof(uint8_t));
    for (uint_t idx = 0; idx < port_ing->get_priority_group_id_count(); idx++) {
        cps_api_object_attr_add_u64(ret_obj, BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_ID_LIST,
                                    port_ing->get_priority_group_id(idx));
    }

    return cps_api_ret_code_OK;
}

/*
  * This function provides NAS-QoS PORT-INGRESS CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_ingress_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    if (obj == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Object not exist\n");
        return cps_api_ret_code_ERR;
    }
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&port_ing_mutex);

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_port_ing_set(obj, NULL);
    }

    // create/delete are not allowed for queue, no roll-back is needed

    return cps_api_ret_code_OK;
}

/* Debugging and unit testing */
void dump_nas_qos_port_ingress(nas_switch_id_t switch_id)
{
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);

    if (p_switch) {
        p_switch->dump_all_port_ing_profile();
    }
}
