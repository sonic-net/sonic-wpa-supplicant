
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
 * \file   nas_qos_policer.h
 * \brief  NAS QOS policer (a.k.a. meter) object
 * \date   02-2015
 * \author
 */

#ifndef _NAS_QOS_POLICER_H_
#define _NAS_QOS_POLICER_H_

#include <unordered_map>

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"
#include "cps_api_object.h"
#include "nas_ndi_obj_id_table.h"

class nas_qos_switch;


typedef struct cps_nas_qos_policer_struct {
    nas_obj_id_t         policer_id; // NAS policer id
    qos_policer_struct_t ndi_cfg;
} cps_nas_qos_policer_struct_t;

class nas_qos_policer : public nas::base_obj_t
{

    // cfg contains the values of all modified attributes
    cps_nas_qos_policer_struct_t cfg;

    // List of mapped NDI IDs one for each NPU
    // managed by this NAS component
    nas::ndi_obj_id_table_t        _ndi_obj_ids;

public:

    nas_qos_policer (nas_qos_switch* switch_p);

    const nas_qos_switch& get_switch() ;

    nas_obj_id_t  policer_id() {return cfg.policer_id;}
    void          set_policer_id(nas_obj_id_t id) {cfg.policer_id = id;}

    const cps_nas_qos_policer_struct_t& get_cfg() const {return cfg;}
    void set_cfg(cps_nas_qos_policer_struct_t & new_cfg) {cfg = new_cfg;}

    bool  opaque_data_to_cps (cps_api_object_t cps_obj) const;
    bool  opaque_data_equal(nas_qos_policer * p);

    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS Policer";}

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

    void poll_stats(policer_stats_struct_t &stats);

    ndi_obj_id_t ndi_obj_id(npu_id_t npu_id) const;
} ;

inline ndi_obj_id_t nas_qos_policer::ndi_obj_id (npu_id_t npu_id) const
{
    return (_ndi_obj_ids.at (npu_id));
}

inline bool     nas_qos_policer::opaque_data_equal(nas_qos_policer * p)
{
    return (_ndi_obj_ids == p->_ndi_obj_ids);
}

#endif
