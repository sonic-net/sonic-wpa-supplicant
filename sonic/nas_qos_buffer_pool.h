
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
 * \file   nas_qos_buffer_pool.h
 * \brief  NAS QOS Buffer Pool object
 */

#ifndef _NAS_QOS_BUFFER_POOL_H_
#define _NAS_QOS_BUFFER_POOL_H_

#include <unordered_map>

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"

class nas_qos_switch;

class nas_qos_buffer_pool : public nas::base_obj_t
{

    // keys
    nas_obj_id_t buffer_pool_id;

    // attributes
    qos_buffer_pool_struct_t cfg;

    ///// Typedefs /////
    typedef std::unordered_map <npu_id_t, ndi_obj_id_t> ndi_obj_id_map_t;
    typedef ndi_obj_id_map_t::iterator ndi_obj_id_map_it_t;

    // List of mapped NDI IDs one for each NPU
    // managed by this NAS component
    ndi_obj_id_map_t          _ndi_obj_ids;

public:

    nas_qos_buffer_pool (nas_qos_switch* switch_p);

    const nas_qos_switch& get_switch() ;

    nas_obj_id_t get_buffer_pool_id() const {return buffer_pool_id;}
    void    set_buffer_pool_id(nas_obj_id_t id) {buffer_pool_id = id;}

    // shared_size is READ-ONLY
    uint32_t    get_shared_size()  const   {return cfg.shared_size;}

    uint32_t    get_size() const {return cfg.size;}
    void        set_size(uint32_t size) {cfg.size = size;}

    BASE_QOS_BUFFER_POOL_TYPE_t get_type() const {return cfg.type;}
    void  set_type(BASE_QOS_BUFFER_POOL_TYPE_t type) {cfg.type = type;}

    BASE_QOS_BUFFER_THRESHOLD_MODE_t get_threshold_mode() const {return cfg.threshold_mode;}
    void  set_threshold_mode(BASE_QOS_BUFFER_THRESHOLD_MODE_t mode) {
        cfg.threshold_mode = mode;
    }

    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS buffer_pool";}

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

    bool  get_first_npu_id(npu_id_t &npu_id);

} ;

inline ndi_obj_id_t nas_qos_buffer_pool::ndi_obj_id (npu_id_t npu_id) const
{
    return (_ndi_obj_ids.at (npu_id));
}

inline void nas_qos_buffer_pool::set_ndi_obj_id (npu_id_t npu_id,
                                           ndi_obj_id_t id)
{
    // Will overwrite or insert a new element
    // if npu_id is not already present.
    _ndi_obj_ids [npu_id] = id;
}

inline void nas_qos_buffer_pool::reset_ndi_obj_id (npu_id_t npu_id)
{
    _ndi_obj_ids.erase (npu_id);
}

inline bool nas_qos_buffer_pool::get_first_npu_id(npu_id_t &npu_id)
{
    for (ndi_obj_id_map_it_t it = _ndi_obj_ids.begin();
            it != _ndi_obj_ids.end();
            it++) {
        npu_id = it->first;
        return true;
    }

    return false;
}

#endif
