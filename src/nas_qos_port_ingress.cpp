
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


#include "event_log.h"
#include "std_assert.h"
#include "nas_qos_common.h"
#include "nas_qos_port_ingress.h"
#include "dell-base-qos.h"
#include "nas_ndi_qos.h"
#include "nas_base_obj.h"
#include "nas_qos_switch.h"

nas_qos_port_ingress::nas_qos_port_ingress (nas_qos_switch* switch_p,
                            hal_ifindex_t port)
           : base_obj_t(switch_p), port_id(port)
{
    memset(&cfg, 0, sizeof(cfg));
    ndi_port_id = {0}; // for coverity check only
}

const nas_qos_switch& nas_qos_port_ingress::get_switch()
{
    return static_cast<const nas_qos_switch&> (base_obj_t::get_switch());
}


void nas_qos_port_ingress::commit_create (bool rolling_back)

{
}

void* nas_qos_port_ingress::alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m)
{
    // NAS Qos port ingress does not allocate memory to save the incoming tentative attributes
    return this;
}

bool nas_qos_port_ingress::push_create_obj_to_npu (npu_id_t npu_id,
                                                   void* ndi_obj)
{
    return true;
}


bool nas_qos_port_ingress::push_delete_obj_to_npu (npu_id_t npu_id)
{
    return true;
}

bool nas_qos_port_ingress::is_leaf_attr (nas_attr_id_t attr_id)
{
    // Table of function pointers to handle modify of Qos port ingress
    // attributes.
    static const std::unordered_map <BASE_QOS_PORT_INGRESS_t,
                                     bool,
                                     std::hash<int>>
        _leaf_attr_map =
    {
        // modifiable objects
        {BASE_QOS_PORT_INGRESS_DEFAULT_TRAFFIC_CLASS,   true},
        {BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_MAP,         true},
        {BASE_QOS_PORT_INGRESS_DOT1P_TO_COLOR_MAP,      true},
        {BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP,   true},
        {BASE_QOS_PORT_INGRESS_DSCP_TO_TC_MAP,          true},
        {BASE_QOS_PORT_INGRESS_DSCP_TO_COLOR_MAP,       true},
        {BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP,    true},
        {BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP,         true},
        {BASE_QOS_PORT_INGRESS_FLOW_CONTROL,            true},
        {BASE_QOS_PORT_INGRESS_POLICER_ID,              true},
        {BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL,     true},
        {BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL, true},
        {BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL, true},
        {BASE_QOS_PORT_INGRESS_TC_TO_PRIORITY_GROUP_MAP,  true},
        {BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP, true},
    };

    return (_leaf_attr_map.at(static_cast<BASE_QOS_PORT_INGRESS_t>(attr_id)));
}

ndi_obj_id_t nas_qos_port_ingress::nas2ndi_map_id(nas_obj_id_t map_id)
{
    if (map_id == 0LL) {
        //map_id 0 is used to remove map from port
        return NDI_QOS_NULL_OBJECT_ID;
    }
    nas_qos_switch& switch_r = const_cast<nas_qos_switch&>(get_switch());
    nas_qos_map* p_map = switch_r.get_map(map_id);
    if (p_map == NULL) {
        t_std_error rc = STD_ERR(QOS, FAIL, 0);
        throw nas::base_exception{rc, __PRETTY_FUNCTION__,
                "Map id is not created in ndi yet"};
    }

    return p_map->ndi_obj_id(ndi_port_id.npu_id);
}

ndi_obj_id_t nas_qos_port_ingress::nas2ndi_policer_id(nas_obj_id_t policer_id)
{
    if (policer_id == 0LL) {
        //policer_id 0 is used to remove policer from port
        return NDI_QOS_NULL_OBJECT_ID;
    }
    nas_qos_switch& switch_r = const_cast<nas_qos_switch&>(get_switch());
    nas_qos_policer* p_policer = switch_r.get_policer(policer_id);
    if (p_policer == NULL) {
        t_std_error rc = STD_ERR(QOS, FAIL, 0);
        throw nas::base_exception{rc, __PRETTY_FUNCTION__,
                "Policer id is not created in ndi yet"};
    }

    return p_policer->ndi_obj_id(ndi_port_id.npu_id);
}

bool nas_qos_port_ingress::push_leaf_attr_to_npu(nas_attr_id_t attr_id,
                                                 npu_id_t npu_id)
{
    t_std_error rc = STD_ERR_OK;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Modifying npu: %d, attr_id %d",
                    npu_id, attr_id);

    qos_port_ing_struct_t cfg= {0};

    switch (attr_id) {
    case BASE_QOS_PORT_INGRESS_DEFAULT_TRAFFIC_CLASS:
        cfg.default_tc = get_default_traffic_class();
        break;
    case BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_MAP:
        cfg.dot1p_to_tc_map = nas2ndi_map_id(get_dot1p_to_tc_map());
        break;
    case BASE_QOS_PORT_INGRESS_DOT1P_TO_COLOR_MAP:
        cfg.dot1p_to_color_map = nas2ndi_map_id(get_dot1p_to_color_map());
        break;
    case BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP:
        cfg.dot1p_to_tc_color_map = nas2ndi_map_id(get_dot1p_to_tc_color_map());
        break;
    case BASE_QOS_PORT_INGRESS_DSCP_TO_TC_MAP:
        cfg.dscp_to_tc_map = nas2ndi_map_id(get_dscp_to_tc_map());
        break;
    case BASE_QOS_PORT_INGRESS_DSCP_TO_COLOR_MAP:
        cfg.dscp_to_color_map = nas2ndi_map_id(get_dscp_to_color_map());
        break;
    case BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP:
        cfg.dscp_to_tc_color_map = nas2ndi_map_id(get_dscp_to_tc_color_map());
        break;
    case BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP:
        cfg.tc_to_queue_map = nas2ndi_map_id(get_tc_to_queue_map());
        break;
    case BASE_QOS_PORT_INGRESS_FLOW_CONTROL:
        cfg.flow_control = (BASE_QOS_FLOW_CONTROL_t)get_flow_control();
        break;
    case BASE_QOS_PORT_INGRESS_POLICER_ID:
        cfg.policer_id = nas2ndi_policer_id(get_policer_id());
        break;
    case BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL:
        cfg.flood_storm_control =
                nas2ndi_policer_id(get_flood_storm_control());
        break;
    case BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL:
        cfg.bcast_storm_control =
                nas2ndi_policer_id(get_broadcast_storm_control());
        break;
    case BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL:
        cfg.mcast_storm_control =
                nas2ndi_policer_id(get_multicast_storm_control());
        break;
    case BASE_QOS_PORT_INGRESS_TC_TO_PRIORITY_GROUP_MAP:
        cfg.tc_to_priority_group_map =
                nas2ndi_map_id(get_tc_to_priority_group_map());
        break;
    case BASE_QOS_PORT_INGRESS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP:
        cfg.priority_group_to_pfc_priority_map =
                nas2ndi_map_id(get_priority_group_to_pfc_priority_map());
        break;
    default:
        STD_ASSERT(0);  //non-modifiable object
    }

    ndi_port_t ndi_port = get_ndi_port_id();
    rc = ndi_qos_set_port_ing_profile_attr(ndi_port.npu_id,
                               ndi_port.npu_port,
                               (BASE_QOS_PORT_INGRESS_t)attr_id,
                               &cfg);
    if (rc != STD_ERR_OK) {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI attribute Set Failed"};
    }

    return true;
}
