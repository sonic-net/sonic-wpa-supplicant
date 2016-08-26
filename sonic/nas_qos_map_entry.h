
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
 * \file   nas_qos_map_entry.h
 * \brief  NAS QOS dot1p to tc and/or color map object
 */

#ifndef _NAS_QOS_MAP_ENTRY_H_
#define _NAS_QOS_MAP_ENTRY_H_


#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"

#include <unordered_map>

class nas_qos_switch;


class nas_qos_map_entry : public nas::base_obj_t
{
    // The map-id/map-type that holds this map entry.
    nas_obj_id_t            map_id;
    nas_qos_map_type_t      type;

    // map entry key: dot1p or dscp or tc
    nas_qos_map_entry_key_t key;

    nas_qos_map_entry_value_t value;

public:

    nas_qos_map_entry (nas_qos_switch* switch_p,
                        nas_obj_id_t map_id,
                        nas_qos_map_type_t type,
                        nas_qos_map_entry_key_t key);
    const nas_qos_switch& get_switch() ;

    nas_qos_map_type_t get_type() const {return type;}

    nas_obj_id_t  get_map_id() const {return map_id;}
    void          set_map_id(nas_obj_id_t id) {map_id = id;}

    t_std_error  get_ndi_map_id(npu_id_t npu_id, ndi_obj_id_t *ndi_map_id);

    nas_qos_map_entry_key_t get_key() const {return key;}

    void    set_value_tc(uint_t val) {
        value.tc = val;
        if (type == NDI_QOS_MAP_DOT1P_TO_TC_COLOR)
            mark_attr_dirty(BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_TC);
        else if (type == NDI_QOS_MAP_DOT1P_TO_TC)
            mark_attr_dirty(BASE_QOS_DOT1P_TO_TC_MAP_ENTRY_TC);
        else if (type == NDI_QOS_MAP_DSCP_TO_TC_COLOR)
            mark_attr_dirty(BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_TC);
        else if (type == NDI_QOS_MAP_DSCP_TO_TC)
            mark_attr_dirty(BASE_QOS_DSCP_TO_TC_MAP_ENTRY_TC);

    }

    void     set_value_color(BASE_QOS_PACKET_COLOR_t val) {
        value.color = val;
        if (type == NDI_QOS_MAP_DOT1P_TO_TC_COLOR)
            mark_attr_dirty(BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_COLOR);
        else if (type == NDI_QOS_MAP_DOT1P_TO_COLOR)
            mark_attr_dirty(BASE_QOS_DOT1P_TO_COLOR_MAP_ENTRY_COLOR);
        else if (type == NDI_QOS_MAP_DSCP_TO_TC_COLOR)
            mark_attr_dirty(BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_COLOR);
        else if (type == NDI_QOS_MAP_DSCP_TO_COLOR)
            mark_attr_dirty(BASE_QOS_DSCP_TO_COLOR_MAP_ENTRY_COLOR);
    }

    void    set_value_dot1p(uint_t val) {
        value.dot1p = val;
        if (type == NDI_QOS_MAP_TC_TO_DOT1P)
            mark_attr_dirty(BASE_QOS_TC_TO_DOT1P_MAP_ENTRY_DOT1P);
        else if (type == NDI_QOS_MAP_TC_COLOR_TO_DOT1P)
            mark_attr_dirty(BASE_QOS_TC_COLOR_TO_DOT1P_MAP_ENTRY_DOT1P);
    }

    void    set_value_dscp(uint_t val) {
        value.dscp = val;
        if (type == NDI_QOS_MAP_TC_TO_DSCP)
            mark_attr_dirty(BASE_QOS_TC_TO_DSCP_MAP_ENTRY_DSCP);
        else if (type == NDI_QOS_MAP_TC_COLOR_TO_DSCP)
            mark_attr_dirty(BASE_QOS_TC_COLOR_TO_DSCP_MAP_ENTRY_DSCP);
    }

    void    set_value_queue_num(uint_t val) {
        value.queue_num = val;
        if (type == NDI_QOS_MAP_TC_TO_QUEUE)
            mark_attr_dirty(BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_QUEUE_NUMBER);
        else if (type == NDI_QOS_MAP_PFC_TO_QUEUE)
            mark_attr_dirty(BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_ENTRY_QUEUE_NUMBER);
    }

    void    set_value_pg(uint_t val) {
        value.pg = val;
        if (type == NDI_QOS_MAP_TC_TO_PG)
            mark_attr_dirty(BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_ENTRY_PRIORITY_GROUP);
    }

    nas_qos_map_entry_value_t get_value() const {return value;}
    t_std_error get_ndi_qid(ndi_obj_id_t &ndi_qid); //TC-to-queue mapping only

    /// Overriding base object virtual functions
    const char* name () const override { return "QOS map dot1p to tc entry";}

    /////// Override for object specific behavior ////////
    void        commit_create (bool rolling_back) ;
    void* alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m) override {return this;}
    bool push_create_obj_to_npu (npu_id_t npu_id,
                                 void* ndi_obj) override;

    bool push_delete_obj_to_npu (npu_id_t npu_id) override;

    bool push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                npu_id_t npu_id) override;
    e_event_log_types_enums ev_log_mod_id () const override {return ev_log_t_QOS;}
    const char* ev_log_mod_name () const override {return "QOS";}


} ;


#endif
