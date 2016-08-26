
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


#ifndef _NAS_QOS_PORT_EGRESS_H_
#define _NAS_QOS_PORT_EGRESS_H_

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"

class nas_qos_switch;

class nas_qos_port_egress : public nas::base_obj_t
{
    // keys
    hal_ifindex_t port_id;

    // attributes
    qos_port_egr_struct_t cfg;

    // queue id
    std::vector<ndi_obj_id_t> _q_id_vec;

    // cached info
    ndi_port_t ndi_port_id;

    ndi_obj_id_t nas2ndi_map_id(nas_obj_id_t map_id);
    ndi_obj_id_t nas2ndi_wred_profile_id(nas_obj_id_t wred_profile_id);
    ndi_obj_id_t nas2ndi_scheduler_profile_id(nas_obj_id_t scheduler_profile_id);

public:
    nas_qos_port_egress(nas_qos_switch* switch_p, hal_ifindex_t port_id);

    nas_qos_switch& get_switch();

    void set_ndi_port_id(npu_id_t npu_id, npu_port_t npu_port_id);
    ndi_port_t get_ndi_port_id() const {return ndi_port_id;}

    hal_ifindex_t get_port_id() const { return port_id; }
    void set_port_id(hal_ifindex_t port) { port_id = port; }

    uint64_t get_buffer_limit() const { return cfg.buffer_limit; }
    void set_buffer_limit(uint64_t limit) { cfg.buffer_limit = limit; }

    ndi_obj_id_t get_wred_profile_id() const { return cfg.wred_profile_id; }
    void set_wred_profile_id(ndi_obj_id_t profile_id) { cfg.wred_profile_id = profile_id; }

    ndi_obj_id_t get_scheduler_profile_id() const { return cfg.scheduler_profile_id; }
    void set_scheduler_profile_id(ndi_obj_id_t profile_id) { cfg.scheduler_profile_id = profile_id; }

    uint8_t get_num_unicast_queue() const { return cfg.num_ucast_queue; }
    void set_num_unicast_queue(uint8_t num) { cfg.num_ucast_queue = num; }

    uint8_t get_num_multicast_queue() const { return cfg.num_mcast_queue; }
    void set_num_multicast_queue(uint8_t num) { cfg.num_mcast_queue = num; }

    uint8_t get_num_queue() const { return cfg.num_queue; }
    void set_num_queue(uint8_t num) { cfg.num_queue = num; }

    uint32_t get_queue_id_count() const { return cfg.num_queue_id; }
    ndi_obj_id_t get_queue_id(uint32_t idx) const {
        return cfg.queue_id_list ? cfg.queue_id_list[idx] : 0LL;
    }
    void add_queue_id(ndi_obj_id_t queue_id)
    {
        _q_id_vec.push_back(queue_id);
        cfg.num_queue_id = _q_id_vec.size();
        cfg.queue_id_list = &_q_id_vec[0];
    }
    void clear_queue_id()
    {
        _q_id_vec.clear();
        cfg.num_queue_id = 0;
        cfg.queue_id_list = &_q_id_vec[0];
    }

    ndi_obj_id_t get_tc_to_queue_map() const { return cfg.tc_to_queue_map; }
    void set_tc_to_queue_map(ndi_obj_id_t map_id) { cfg.tc_to_queue_map = map_id; }

    ndi_obj_id_t get_tc_to_dot1p_map() const { return cfg.tc_to_dot1p_map; }
    void set_tc_to_dot1p_map(ndi_obj_id_t map_id) { cfg.tc_to_dot1p_map = map_id; }

    ndi_obj_id_t get_tc_to_dscp_map() const { return cfg.tc_to_dscp_map; }
    void set_tc_to_dscp_map(ndi_obj_id_t map_id) { cfg.tc_to_dscp_map = map_id; }

    ndi_obj_id_t get_tc_color_to_dot1p_map() const { return cfg.tc_color_to_dot1p_map; }
    void set_tc_color_to_dot1p_map(ndi_obj_id_t map_id) { cfg.tc_color_to_dot1p_map = map_id; }

    ndi_obj_id_t get_tc_color_to_dscp_map() const { return cfg.tc_color_to_dscp_map; }
    void set_tc_color_to_dscp_map(ndi_obj_id_t map_id) { cfg.tc_color_to_dscp_map = map_id; }

    ndi_obj_id_t get_pfc_priority_to_queue_map() const { return cfg.pfc_priority_to_queue_map;}
    void set_pfc_priority_to_queue_map(ndi_obj_id_t map_id) { cfg.pfc_priority_to_queue_map = map_id; }


    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS PORT EGRESS";}

    /////// Override for object specific behavior ////////
    // Commit newly created object
    virtual void        commit_create (bool rolling_back);

    virtual void* alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m) override;
    virtual bool push_create_obj_to_npu (npu_id_t npu_id,
                                         void* ndi_obj) override;

    virtual bool push_delete_obj_to_npu (npu_id_t npu_id) override;

    virtual bool is_leaf_attr (nas_attr_id_t attr_id);
    virtual bool push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                        npu_id_t npu_id) override;
    virtual e_event_log_types_enums ev_log_mod_id () const override {return ev_log_t_QOS;}
    virtual const char* ev_log_mod_name () const override {return "QOS";}
} ;

inline void nas_qos_port_egress::set_ndi_port_id(npu_id_t npu_id,
                                                npu_port_t npu_port_id)
{
    ndi_port_id.npu_id = npu_id;
    ndi_port_id.npu_port = npu_port_id;
}

/* This function pre-loads the NAS QoS module with default QoS Port-egress info
 * @Return standard error code
 */
t_std_error nas_qos_port_egress_init();

/* Debugging and unit testing */
void dump_nas_qos_port_egress(nas_switch_id_t switch_id);

#endif
