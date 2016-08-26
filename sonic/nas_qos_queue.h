
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
 * \file   nas_qos_queue.h
 * \brief  NAS QOS queue object
 */

#ifndef _NAS_QOS_QUEUE_H_
#define _NAS_QOS_QUEUE_H_

#include <unordered_map>

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "dell-base-qos.h"
#include "nas_ndi_obj_id_table.h"

class nas_qos_switch;

typedef struct nas_qos_queue_key_t {
    hal_ifindex_t           port_id;
    uint32_t                local_queue_id;
    BASE_QOS_QUEUE_TYPE_t   type;

    bool operator<(const nas_qos_queue_key_t key) const {
        if (port_id != key.port_id)
            return (port_id < key.port_id);

        if (local_queue_id != key.local_queue_id)
            return (local_queue_id < key.local_queue_id);

        return (type < key.type);
    }

} nas_qos_queue_key_t;


class nas_qos_queue : public nas::base_obj_t
{
    // key
    nas_qos_queue_key_t key;

    // attributes
    nas_obj_id_t    queue_id;
    nas_obj_id_t    wred_id;
    nas_obj_id_t    buffer_profile;
    nas_obj_id_t    scheduler_profile;

    nas_obj_id_t    parent_id;

    // cached info
    ndi_port_t           ndi_port_id; // derived from nas_queue_key.port_id, i.e. ifIndex

    // List of mapped NDI IDs (Only one for each queue)
    // managed by this NAS component
    nas::ndi_obj_id_table_t        _ndi_obj_ids;



public:

    nas_qos_queue (nas_qos_switch* switch_p, nas_qos_queue_key_t key);

    const nas_qos_switch& get_switch() ;

    void        set_ndi_port_id(npu_id_t npu_id, npu_port_t npu_port_id);
    ndi_port_t     get_ndi_port_id() {return ndi_port_id;}

    nas_obj_id_t  get_queue_id() {return queue_id;}
    void           set_queue_id(nas_obj_id_t val) {queue_id = val;}

    nas_obj_id_t get_queue_parent_id() {return parent_id;}
    void    set_queue_parent_id(nas_obj_id_t id) {parent_id = id;}
    bool    is_queue_attached() {
        return (parent_id != NDI_QOS_NULL_OBJECT_ID);
    }

    nas_qos_queue_key_t get_key() {return key;}
    hal_ifindex_t  get_port_id() const {return key.port_id;}
    BASE_QOS_QUEUE_TYPE_t get_type() const {return key.type;}
    uint32_t get_local_queue_id() const {return key.local_queue_id;}

    // User configurable attributes
    bool is_wred_id_set() { return _set_attributes.contains(BASE_QOS_QUEUE_WRED_ID);}
    nas_obj_id_t get_wred_id() const {return wred_id;}
    void     set_wred_id(nas_obj_id_t id);

    bool is_buffer_profile_set() {return _set_attributes.contains(BASE_QOS_QUEUE_BUFFER_PROFILE_ID);}
    nas_obj_id_t get_buffer_profile() const {return buffer_profile;}
    void    set_buffer_profile(nas_obj_id_t id);

    bool is_scheduler_profile_set() {return _set_attributes.contains(BASE_QOS_QUEUE_SCHEDULER_PROFILE_ID);}
    nas_obj_id_t get_scheduler_profile() const {return scheduler_profile;}
    void    set_scheduler_profile(nas_obj_id_t id);

    bool  opaque_data_to_cps (cps_api_object_t cps_obj) const;

    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS queue";}

    /////// Override for object specific behavior ////////
    // Commit newly created object
    virtual void        commit_create (bool rolling_back) override;

    virtual void* alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m) override;
    virtual bool push_create_obj_to_npu (npu_id_t npu_id,
                                         void* ndi_obj) override;

    virtual bool push_delete_obj_to_npu (npu_id_t npu_id) override;

    virtual bool is_leaf_attr (nas_attr_id_t attr_id);
    virtual bool push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                        npu_id_t npu_id) override;
    virtual e_event_log_types_enums ev_log_mod_id () const override {return ev_log_t_QOS;}
    virtual const char* ev_log_mod_name () const override {return "QOS";}

    ndi_obj_id_t      ndi_obj_id () const;
    void set_ndi_obj_id (ndi_obj_id_t obj_id);
    void reset_ndi_obj_id ();

} ;

inline ndi_obj_id_t nas_qos_queue::ndi_obj_id () const
{
    return (_ndi_obj_ids.at (ndi_port_id.npu_id));
}

inline void nas_qos_queue::set_ndi_obj_id (ndi_obj_id_t id)
{
    _ndi_obj_ids[ndi_port_id.npu_id] = id;
}

inline void nas_qos_queue::reset_ndi_obj_id ()
{
    _ndi_obj_ids[ndi_port_id.npu_id] = 0;
}

inline void nas_qos_queue::set_ndi_port_id(npu_id_t npu_id, npu_port_t npu_port_id)
{
    ndi_port_id.npu_id = npu_id;
    ndi_port_id.npu_port = npu_port_id;
}

inline void nas_qos_queue::set_wred_id(nas_obj_id_t id)
{
    mark_attr_dirty(BASE_QOS_QUEUE_WRED_ID);
    wred_id = id;
}

inline void    nas_qos_queue::set_buffer_profile(nas_obj_id_t id)
{
    mark_attr_dirty(BASE_QOS_QUEUE_BUFFER_PROFILE_ID);
    buffer_profile = id;
}

inline void nas_qos_queue::set_scheduler_profile(nas_obj_id_t id)
{
    mark_attr_dirty(BASE_QOS_QUEUE_SCHEDULER_PROFILE_ID);
    scheduler_profile = id;
}

/* Debugging and unit testing */
void dump_nas_qos_queues(uint_t switch_id);


#endif
