
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
#include "nas_qos_port_egress.h"
#include "dell-base-qos.h"
#include "nas_ndi_qos.h"
#include "nas_base_obj.h"
#include "nas_qos_switch.h"

nas_qos_port_egress::nas_qos_port_egress (nas_qos_switch* switch_p,
                            hal_ifindex_t port)
           : base_obj_t(switch_p), port_id(port)
{
    memset(&cfg, 0, sizeof(cfg));
    ndi_port_id = {0}; // for coverity check only
}

nas_qos_switch& nas_qos_port_egress::get_switch()
{
    return static_cast<nas_qos_switch &>(base_obj_t::get_switch());
}


void nas_qos_port_egress::commit_create (bool rolling_back)

{
}

void* nas_qos_port_egress::alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m)
{
    // NAS Qos port egress does not allocate memory to save the incoming tentative attributes
    return this;
}

bool nas_qos_port_egress::push_create_obj_to_npu (npu_id_t npu_id,
                                                   void* ndi_obj)
{
    return true;
}

bool nas_qos_port_egress::push_delete_obj_to_npu (npu_id_t npu_id)
{
    return true;
}

bool nas_qos_port_egress::is_leaf_attr (nas_attr_id_t attr_id)
{
    // Table of function pointers to handle modify of Qos port ingress
    // attributes.
    static const std::unordered_map <BASE_QOS_PORT_EGRESS_t,
                                     bool,
                                     std::hash<int>>
        _leaf_attr_map =
    {
        // modifiable objects
        {BASE_QOS_PORT_EGRESS_WRED_PROFILE_ID,          true},
        {BASE_QOS_PORT_EGRESS_SCHEDULER_PROFILE_ID,     true},
        {BASE_QOS_PORT_EGRESS_NUM_UNICAST_QUEUE,        true},
        {BASE_QOS_PORT_EGRESS_NUM_MULTICAST_QUEUE,      true},
        {BASE_QOS_PORT_EGRESS_NUM_QUEUE,                true},
        {BASE_QOS_PORT_EGRESS_QUEUE_ID_LIST,            true},
        {BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP,          true},
        {BASE_QOS_PORT_EGRESS_TC_TO_DOT1P_MAP,          true},
        {BASE_QOS_PORT_EGRESS_TC_TO_DSCP_MAP,           true},
        {BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP,    true},
        {BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP,     true},
        {BASE_QOS_PORT_EGRESS_PFC_PRIORITY_TO_QUEUE_MAP,true},
    };

    return (_leaf_attr_map.at(static_cast<BASE_QOS_PORT_EGRESS_t>(attr_id)));
}

ndi_obj_id_t nas_qos_port_egress::nas2ndi_map_id(nas_obj_id_t map_id)
{
    if (map_id == 0LL) {
        //map_id 0 is used to remove map from port
        return NDI_QOS_NULL_OBJECT_ID;
    }
    nas_qos_switch& switch_r = get_switch();
    nas_qos_map* p_map = switch_r.get_map(map_id);
    if (p_map == NULL) {
        t_std_error rc = STD_ERR(QOS, FAIL, 0);
        throw nas::base_exception{rc, __PRETTY_FUNCTION__,
                "Map id is not created in ndi yet"};
    }

    return p_map->ndi_obj_id(ndi_port_id.npu_id);
}

ndi_obj_id_t nas_qos_port_egress::nas2ndi_wred_profile_id(nas_obj_id_t wred_profile_id)
{
    if (wred_profile_id == 0LL) {
        return NDI_QOS_NULL_OBJECT_ID;
    }
    nas_qos_switch& switch_r = get_switch();
    nas_qos_wred* p_wred = switch_r.get_wred(wred_profile_id);
    if (p_wred == NULL) {
        t_std_error rc = STD_ERR(QOS, FAIL, 0);
        throw nas::base_exception{rc, __PRETTY_FUNCTION__,
                "WRED pfoile id is not created in ndi yet"};
    }

    return p_wred->ndi_obj_id(ndi_port_id.npu_id);
}

ndi_obj_id_t nas_qos_port_egress::nas2ndi_scheduler_profile_id(nas_obj_id_t scheduler_profile_id)
{
    if (scheduler_profile_id == 0LL) {
        return NDI_QOS_NULL_OBJECT_ID;
    }
    nas_qos_switch& switch_r = get_switch();
    nas_qos_scheduler* p_sched = switch_r.get_scheduler(scheduler_profile_id);
    if (p_sched == NULL) {
        t_std_error rc = STD_ERR(QOS, FAIL, 0);
        throw nas::base_exception{rc, __PRETTY_FUNCTION__,
                "Scheduler pfoile id is not created in ndi yet"};
    }

    return p_sched->ndi_obj_id(ndi_port_id.npu_id);
}

bool nas_qos_port_egress::push_leaf_attr_to_npu(nas_attr_id_t attr_id,
                                                 npu_id_t npu_id)
{
    t_std_error rc = STD_ERR_OK;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Modifying npu: %d, attr_id %d",
                    npu_id, attr_id);

    qos_port_egr_struct_t cfg;
    memset(&cfg, 0, sizeof(qos_port_egr_struct_t));

    switch (attr_id) {
    case BASE_QOS_PORT_EGRESS_BUFFER_LIMIT:
        cfg.buffer_limit = get_buffer_limit();
        break;
    case BASE_QOS_PORT_EGRESS_WRED_PROFILE_ID:
        cfg.wred_profile_id = nas2ndi_wred_profile_id(get_wred_profile_id());
        break;
    case BASE_QOS_PORT_EGRESS_SCHEDULER_PROFILE_ID:
        cfg.scheduler_profile_id = nas2ndi_scheduler_profile_id(get_scheduler_profile_id());
        break;
    case BASE_QOS_PORT_EGRESS_NUM_UNICAST_QUEUE:
        cfg.num_ucast_queue = get_num_unicast_queue();
        break;
    case BASE_QOS_PORT_EGRESS_NUM_MULTICAST_QUEUE:
        cfg.num_mcast_queue = get_num_multicast_queue();
        break;
    case BASE_QOS_PORT_EGRESS_NUM_QUEUE:
        cfg.num_queue = get_num_queue();
        break;
    case BASE_QOS_PORT_EGRESS_QUEUE_ID_LIST:
    {
        uint32_t queue_id_num, id;
        queue_id_num = get_queue_id_count();
        for (id = 0; id < queue_id_num; id ++) {
            _q_id_vec.push_back(get_queue_id(id));
        }
        cfg.num_queue_id = _q_id_vec.size();
        cfg.queue_id_list = &_q_id_vec[0];
        break;
    }
    case BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP:
        cfg.tc_to_queue_map = nas2ndi_map_id(get_tc_to_queue_map());
        break;
    case BASE_QOS_PORT_EGRESS_TC_TO_DOT1P_MAP:
        cfg.tc_to_dot1p_map = nas2ndi_map_id(get_tc_to_dot1p_map());
        break;
    case BASE_QOS_PORT_EGRESS_TC_TO_DSCP_MAP:
        cfg.tc_to_dscp_map = nas2ndi_map_id(get_tc_to_dscp_map());
        break;
    case BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP:
        cfg.tc_color_to_dot1p_map = nas2ndi_map_id(get_tc_color_to_dot1p_map());
        break;
    case BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP:
        cfg.tc_color_to_dscp_map = nas2ndi_map_id(get_tc_color_to_dscp_map());
        break;
    case BASE_QOS_PORT_EGRESS_PFC_PRIORITY_TO_QUEUE_MAP:
        cfg.pfc_priority_to_queue_map = nas2ndi_map_id(get_pfc_priority_to_queue_map());
        break;
    default:
        STD_ASSERT(0);  //non-modifiable object
    }

    ndi_port_t ndi_port = get_ndi_port_id();
    rc = ndi_qos_set_port_egr_profile_attr(ndi_port.npu_id,
                               ndi_port.npu_port,
                               (BASE_QOS_PORT_EGRESS_t)attr_id,
                               &cfg);
    if (rc != STD_ERR_OK) {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI attribute Set Failed"};
    }

    return true;
}
