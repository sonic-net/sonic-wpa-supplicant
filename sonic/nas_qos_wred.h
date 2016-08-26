
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
 * \file   nas_qos_wred.h
 * \brief  NAS QOS WRED object
 * \date   05-2015
 * \author
 */

#ifndef _NAS_QOS_WRED_H_
#define _NAS_QOS_WRED_H_

#include <unordered_map>

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"

class nas_qos_switch;

class nas_qos_wred : public nas::base_obj_t
{

    // keys
    nas_obj_id_t wred_id;

    // attributes
    qos_wred_struct_t cfg;

    ///// Typedefs /////
    typedef std::unordered_map <npu_id_t, ndi_obj_id_t> ndi_obj_id_map_t;

    // List of mapped NDI IDs one for each NPU
    // managed by this NAS component
    ndi_obj_id_map_t          _ndi_obj_ids;

public:

    nas_qos_wred (nas_qos_switch* switch_p);

    const nas_qos_switch& get_switch() ;

    nas_obj_id_t get_wred_id() const {return wred_id;}
    void    set_wred_id(nas_obj_id_t id) {wred_id = id;}

    bool    get_g_enable()  const  {return cfg.g_enable;}
    void    set_g_enable(bool val) {cfg.g_enable = val;}

    uint_t  get_g_min()  const  {return cfg.g_min;}
    void    set_g_min(uint_t val) {cfg.g_min = val;}

    uint_t  get_g_max()  const  {return cfg.g_max;}
    void    set_g_max(uint_t val) {cfg.g_max = val;}

    uint_t  get_g_drop_prob() const   {return cfg.g_drop_prob;}
    void    set_g_drop_prob(uint_t val) {cfg.g_drop_prob = val;}

    bool    get_y_enable() const   {return cfg.y_enable;}
    void    set_y_enable(bool val) {cfg.y_enable = val;}

    uint_t  get_y_min() const   {return cfg.y_min;}
    void    set_y_min(uint_t val) {cfg.y_min = val;}

    uint_t  get_y_max()  const  {return cfg.y_max;}
    void    set_y_max(uint_t val) {cfg.y_max = val;}

    uint_t  get_y_drop_prob() const   {return cfg.y_drop_prob;}
    void    set_y_drop_prob(uint_t val) {cfg.y_drop_prob = val;}

    bool    get_r_enable()  const  {return cfg.r_enable;}
    void    set_r_enable(bool val) {cfg.r_enable = val;}

    uint_t  get_r_min() const   {return cfg.r_min;}
    void    set_r_min(uint_t val) {cfg.r_min = val;}

    uint_t  get_r_max() const   {return cfg.r_max;}
    void    set_r_max(uint_t val) {cfg.r_max = val;}

    uint_t  get_r_drop_prob()  const  {return cfg.r_drop_prob;}
    void    set_r_drop_prob(uint_t val) {cfg.r_drop_prob = val;}

    uint_t  get_weight() const {return cfg.weight;}
    void    set_weight(uint_t val) {cfg.weight = val;}

    bool    get_ecn_enable()  const  {return cfg.ecn_enable;}
    void    set_ecn_enable(bool val) {cfg.ecn_enable = val;}



    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS WRED";}

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

    ndi_obj_id_t      ndi_obj_id (npu_id_t npu_id) const;
    void set_ndi_obj_id (npu_id_t npu_id,
                         ndi_obj_id_t obj_id);
    void reset_ndi_obj_id (npu_id_t npu_id);

} ;

inline ndi_obj_id_t nas_qos_wred::ndi_obj_id (npu_id_t npu_id) const
{
    return (_ndi_obj_ids.at (npu_id));
}

inline void nas_qos_wred::set_ndi_obj_id (npu_id_t npu_id,
                                           ndi_obj_id_t id)
{
    // Will overwrite or insert a new element
    // if npu_id is not already present.
    _ndi_obj_ids [npu_id] = id;
}

inline void nas_qos_wred::reset_ndi_obj_id (npu_id_t npu_id)
{
    _ndi_obj_ids.erase (npu_id);
}


#endif
