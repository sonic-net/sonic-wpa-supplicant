
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


#ifndef _NAS_QOS_PORT_INGRESS_H_
#define _NAS_QOS_PORT_INGRESS_H_

#include <unordered_map>

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"

class nas_qos_switch;

class nas_qos_port_ingress : public nas::base_obj_t
{
    // keys
    hal_ifindex_t port_id;

    // attributes
    qos_port_ing_struct_t cfg;

    // priority group id
    std::vector<ndi_obj_id_t> _pg_id_vec;

    // cached info
    ndi_port_t ndi_port_id;

    ndi_obj_id_t nas2ndi_map_id(nas_obj_id_t map_id);

    ndi_obj_id_t nas2ndi_policer_id(nas_obj_id_t policer_id);

public:
    nas_qos_port_ingress(nas_qos_switch* switch_p, hal_ifindex_t port_id);

    const nas_qos_switch& get_switch();

    void set_ndi_port_id(npu_id_t npu_id, npu_port_t npu_port_id);
    ndi_port_t get_ndi_port_id() const {return ndi_port_id;}

    hal_ifindex_t get_port_id() const { return port_id; }
    void set_port_id(hal_ifindex_t port) { port_id = port; }

    uint_t get_default_traffic_class() const { return cfg.default_tc; }
    void set_default_traffic_class(uint_t traf_class) { cfg.default_tc = traf_class; }

    ndi_obj_id_t get_dot1p_to_tc_map() const { return cfg.dot1p_to_tc_map; }
    void set_dot1p_to_tc_map(ndi_obj_id_t map_id) { cfg.dot1p_to_tc_map = map_id; }

    ndi_obj_id_t get_dot1p_to_color_map() const { return cfg.dot1p_to_color_map; }
    void set_dot1p_to_color_map(ndi_obj_id_t map_id) { cfg.dot1p_to_color_map = map_id; }

    ndi_obj_id_t get_dot1p_to_tc_color_map() const { return cfg.dot1p_to_tc_color_map; }
    void set_dot1p_to_tc_color_map(ndi_obj_id_t map_id)
    {
        cfg.dot1p_to_tc_color_map = map_id;
    }

    ndi_obj_id_t get_dscp_to_tc_map() const { return cfg.dscp_to_tc_map; }
    void set_dscp_to_tc_map(ndi_obj_id_t map_id) { cfg.dscp_to_tc_map = map_id; }

    ndi_obj_id_t get_dscp_to_color_map() const { return cfg.dscp_to_color_map; }
    void set_dscp_to_color_map(ndi_obj_id_t map_id) { cfg.dscp_to_color_map = map_id; }

    ndi_obj_id_t get_dscp_to_tc_color_map() const { return cfg.dscp_to_tc_color_map; }
    void set_dscp_to_tc_color_map(ndi_obj_id_t map_id)
    {
        cfg.dscp_to_tc_color_map = map_id;
    }

    ndi_obj_id_t get_tc_to_queue_map() const { return cfg.tc_to_queue_map; }
    void set_tc_to_queue_map(ndi_obj_id_t map_id)
    {
        cfg.tc_to_queue_map = map_id;
    }

    uint_t get_flow_control() const { return cfg.flow_control; }
    void set_flow_control(uint_t flow_control) {
        cfg.flow_control = (BASE_QOS_FLOW_CONTROL_t)flow_control;
    }

    ndi_obj_id_t get_policer_id() const { return cfg.policer_id; }
    void set_policer_id(ndi_obj_id_t id) { cfg.policer_id = id; }

    ndi_obj_id_t get_flood_storm_control() const { return cfg.flood_storm_control; }
    void set_flood_storm_control(ndi_obj_id_t control_id)
    {
        cfg.flood_storm_control = control_id;
    }

    ndi_obj_id_t get_broadcast_storm_control() const { return cfg.bcast_storm_control; }
    void set_broadcast_storm_control(ndi_obj_id_t control_id)
    {
        cfg.bcast_storm_control = control_id;
    }

    ndi_obj_id_t get_multicast_storm_control() const { return cfg.mcast_storm_control; }
    void set_multicast_storm_control(ndi_obj_id_t control_id)
    {
        cfg.mcast_storm_control = control_id;
    }

    uint_t get_priority_group_number() const {return cfg.priority_group_number;}
    void set_priority_group_number(uint_t val) {cfg.priority_group_number = val; }

    // READ-only PG-list
    uint32_t get_priority_group_id_count() const { return cfg.num_priority_group_id; }
    ndi_obj_id_t get_priority_group_id(uint32_t idx) const {
        return cfg.priority_group_id_list ? cfg.priority_group_id_list[idx] : 0LL;
    }
    void add_priority_group_id(ndi_obj_id_t priority_group_id)
    {
        _pg_id_vec.push_back(priority_group_id);
        cfg.num_priority_group_id = _pg_id_vec.size();
        cfg.priority_group_id_list = &_pg_id_vec[0];
    }
    void clear_priority_group_id()
    {
        _pg_id_vec.clear();
        cfg.num_priority_group_id = 0;
        cfg.priority_group_id_list = &_pg_id_vec[0];
    }

    uint8_t get_per_priority_flow_control() const {return cfg.per_priority_flow_control;};
    void set_per_priority_flow_control(uint8_t bit_vec)
    {
        cfg.per_priority_flow_control = bit_vec;
    }

    ndi_obj_id_t get_tc_to_priority_group_map() const { return cfg.tc_to_priority_group_map; }
    void set_tc_to_priority_group_map(ndi_obj_id_t map_id)
    {
        cfg.tc_to_priority_group_map = map_id;
    }

    ndi_obj_id_t get_priority_group_to_pfc_priority_map() const { return cfg.priority_group_to_pfc_priority_map; }
    void set_priority_group_to_pfc_priority_map(ndi_obj_id_t map_id)
    {
        cfg.priority_group_to_pfc_priority_map = map_id;
    }


    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS PORT INGRESS";}

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

inline void nas_qos_port_ingress::set_ndi_port_id(npu_id_t npu_id,
                                                npu_port_t npu_port_id)
{
    ndi_port_id.npu_id = npu_id;
    ndi_port_id.npu_port = npu_port_id;
}

/* This function pre-loads the NAS QoS module with default QoS Port-ingress info
 * @Return standard error code
 */
t_std_error nas_qos_port_ingress_init();

/* Debugging and unit testing */
void dump_nas_qos_port_ingress(nas_switch_id_t switch_id);

#endif
