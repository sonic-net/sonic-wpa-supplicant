
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
 * \file   nas_qos_map.h
 * \brief  NAS QOS map object
 * \date   05-2015
 * \author
 */

#ifndef _NAS_QOS_MAP_H_
#define _NAS_QOS_MAP_H_

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"
#include "nas_qos_map_entry.h"
#include "dell-base-qos.h" // BASE_QOS_QUEUE_TYPE_t

#include <unordered_map>
#include <map>


class nas_qos_switch;

#define nas2ndi_qos_map_type(x) ((ndi_qos_map_type_t)(x))
#define nas2ndi_qos_map_id(x)     ((ndi_obj_id_t)(x))

typedef std::unordered_map<uint_t, nas_qos_map_entry> map_entries;
typedef map_entries::iterator map_entries_iter_t;
typedef map_entries::const_iterator const_map_entries_iter_t;

class nas_qos_map : public nas::base_obj_t
{
    // Keys
    nas_obj_id_t            map_id; // NAS map id

    // Attributes
    nas_qos_map_type_t        type;

    map_entries               map_info;

    ///// Typedefs /////
    typedef std::unordered_map <npu_id_t, ndi_obj_id_t> ndi_obj_id_map_t;

    // List of mapped NDI IDs one for each NPU
    // managed by this NAS component
    ndi_obj_id_map_t          _ndi_obj_ids;

public:

    nas_qos_map (nas_qos_switch* switch_p, nas_qos_map_type_t val);

    const nas_qos_switch& get_switch() ;

    nas_obj_id_t  get_map_id() const {return map_id;}
    void          set_map_id(nas_obj_id_t id) {map_id = id;}

    nas_qos_map_type_t     get_type() const {return type;}


    /// Overriding base object virtual functions
    const char* name () const override { return "QOS map";}

    /////// Override for object specific behavior ////////
    void* alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m) override;
    bool push_create_obj_to_npu (npu_id_t npu_id,
                                 void* ndi_obj) override;

    bool push_delete_obj_to_npu (npu_id_t npu_id) override;

    bool push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                npu_id_t npu_id) override {return true;}
    e_event_log_types_enums ev_log_mod_id () const override {return ev_log_t_QOS;}
    const char* ev_log_mod_name () const override {return "QOS";}

    ndi_obj_id_t      ndi_obj_id (npu_id_t npu_id) const;
    void set_ndi_obj_id (npu_id_t npu_id,
                         ndi_obj_id_t obj_id);
    void reset_ndi_obj_id (npu_id_t npu_id);

    t_std_error add_map_entry(nas_qos_map_entry_key_t key, nas_qos_map_entry &entry) ;
    t_std_error del_map_entry(nas_qos_map_entry_key_t key) ;
    map_entries * get_map_entries();


} ;

inline ndi_obj_id_t nas_qos_map::ndi_obj_id (npu_id_t npu_id) const
{
    return (_ndi_obj_ids.at (npu_id));
}

inline void nas_qos_map::set_ndi_obj_id (npu_id_t npu_id,
                                           ndi_obj_id_t id)
{
    // Will overwrite or insert a new element
    // if npu_id is not already present.
    _ndi_obj_ids [npu_id] = id;
}

inline void nas_qos_map::reset_ndi_obj_id (npu_id_t npu_id)
{
    _ndi_obj_ids.erase (npu_id);
}


#endif
