
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
 * \file   nas_qos_buffer_profile.h
 * \brief  NAS QOS Buffer Profile object
 * \date   03-2016
 * \author
 */

#ifndef _NAS_QOS_BUFFER_PROFILE_H_
#define _NAS_QOS_BUFFER_PROFILE_H_

#include <unordered_map>

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"

class nas_qos_switch;

typedef struct nas_qos_buffer_profile_struct{
    nas_obj_id_t pool_id;         // pool id for the buffer profile
    uint32_t buffer_size;     // reserved buffer size in bytes
    uint32_t shared_dynamic_th; // dynamic threshold for the shared usage: 1/n of available buffer of the pool
    uint32_t shared_static_th;  // static threshold for the shared usage in bytes
    uint32_t xoff_th;   // XOFF threshold in bytes
    uint32_t xon_th;    // XON threshold in bytes
}nas_qos_buffer_profile_struct_t;


class nas_qos_buffer_profile : public nas::base_obj_t
{

    // keys
    nas_obj_id_t buffer_profile_id;

    // attributes
    nas_qos_buffer_profile_struct_t cfg;

    ///// Typedefs /////
    typedef std::unordered_map <npu_id_t, ndi_obj_id_t> ndi_obj_id_map_t;

    // List of mapped NDI IDs one for each NPU
    // managed by this NAS component
    ndi_obj_id_map_t          _ndi_obj_ids;

public:

    nas_qos_buffer_profile (nas_qos_switch* switch_p);

    const nas_qos_switch& get_switch() ;

    nas_obj_id_t get_buffer_profile_id() const {return buffer_profile_id;}
    void    set_buffer_profile_id(nas_obj_id_t id) {buffer_profile_id = id;}

    nas_obj_id_t get_buffer_pool_id() const {return cfg.pool_id;}
    void     set_buffer_pool_id(nas_obj_id_t id) {cfg.pool_id = id;}

    uint32_t get_buffer_size() const {return cfg.buffer_size;}
    void     set_buffer_size(uint32_t size) {cfg.buffer_size = size;}

    uint32_t get_shared_dynamic_th() const {return cfg.shared_dynamic_th;}
    void     set_shared_dynamic_th(uint32_t th) {cfg.shared_dynamic_th = th;}

    uint32_t get_shared_static_th() const { return cfg.shared_static_th;}
    void     set_shared_static_th(uint32_t th) {cfg.shared_static_th = th;}

    uint32_t get_xoff_th() const {return cfg.xoff_th;}
    void     set_xoff_th(uint32_t th) {cfg.xoff_th = th;}

    uint32_t get_xon_th() const {return cfg.xon_th;}
    void     set_xon_th(uint32_t th) {cfg.xon_th = th;}

    ndi_obj_id_t nas2ndi_pool_id(nas_obj_id_t pool_id, npu_id_t npu_id);

    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS buffer_profile";}

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

inline ndi_obj_id_t nas_qos_buffer_profile::ndi_obj_id (npu_id_t npu_id) const
{
    return (_ndi_obj_ids.at (npu_id));
}

inline void nas_qos_buffer_profile::set_ndi_obj_id (npu_id_t npu_id,
                                           ndi_obj_id_t id)
{
    // Will overwrite or insert a new element
    // if npu_id is not already present.
    _ndi_obj_ids [npu_id] = id;
}

inline void nas_qos_buffer_profile::reset_ndi_obj_id (npu_id_t npu_id)
{
    _ndi_obj_ids.erase (npu_id);
}


#endif
