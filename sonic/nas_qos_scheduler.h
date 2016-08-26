
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
 * \file   nas_qos_scheduler.h
 * \brief  NAS QOS scheduler object
 */

#ifndef _NAS_QOS_SCHEDULER_H_
#define _NAS_QOS_SCHEDULER_H_

#include <unordered_map>

#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"

class nas_qos_switch;


class nas_qos_scheduler : public nas::base_obj_t
{
    // Keys
    nas_obj_id_t             scheduler_id; // NAS scheduler id

    // Attributes
    qos_scheduler_struct_t     cfg;

    ///// Typedefs /////
    typedef std::unordered_map <npu_id_t, ndi_obj_id_t> ndi_obj_id_map_t;

    // List of mapped NDI IDs one for each NPU
    // managed by this NAS component
    ndi_obj_id_map_t          _ndi_obj_ids;

public:

    nas_qos_scheduler (nas_qos_switch* switch_p);

    const nas_qos_switch& get_switch() ;

    nas_obj_id_t  get_scheduler_id() const {return scheduler_id;}
    void          set_scheduler_id(nas_obj_id_t id) {scheduler_id = id;}

    BASE_QOS_SCHEDULING_TYPE_t     get_algorithm() const {return cfg.algorithm;}
    void     set_algorithm(BASE_QOS_SCHEDULING_TYPE_t val);

    BASE_QOS_METER_TYPE_t     get_meter_type() const {return cfg.meter_type;}
    void     set_meter_type(BASE_QOS_METER_TYPE_t val);

    uint32_t    get_weight() const {return cfg.weight;}
    void     set_weight(uint32_t val);

    uint64_t    get_min_rate() const { return cfg.min_rate;}
    void     set_min_rate(uint64_t val);

    uint64_t    get_min_burst() const {return cfg.min_burst;}
    void     set_min_burst(uint64_t val);

    uint64_t    get_max_rate() const {return cfg.max_rate;}
    void     set_max_rate(uint64_t val);

    uint64_t    get_max_burst() const {return cfg.max_burst;}
    void     set_max_burst(uint64_t val);


    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS Scheduler";}

    /////// Override for object specific behavior ////////
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

inline ndi_obj_id_t nas_qos_scheduler::ndi_obj_id (npu_id_t npu_id) const
{
    return (_ndi_obj_ids.at (npu_id));
}

inline void nas_qos_scheduler::set_ndi_obj_id (npu_id_t npu_id,
                                           ndi_obj_id_t id)
{
    // Will overwrite or insert a new element
    // if npu_id is not already present.
    _ndi_obj_ids [npu_id] = id;
}

inline void nas_qos_scheduler::reset_ndi_obj_id (npu_id_t npu_id)
{
    _ndi_obj_ids.erase (npu_id);
}

inline void nas_qos_scheduler::set_algorithm(BASE_QOS_SCHEDULING_TYPE_t val)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_PROFILE_ALGORITHM);
    cfg.algorithm = val;
}

inline void nas_qos_scheduler::set_weight(uint32_t val)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_PROFILE_WEIGHT);
    cfg.weight = val;
}

inline void nas_qos_scheduler::set_meter_type(BASE_QOS_METER_TYPE_t val)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_PROFILE_METER_TYPE);
    cfg.meter_type = val;
}


inline void nas_qos_scheduler::set_min_rate(uint64_t val)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_PROFILE_MIN_RATE);
    cfg.min_rate = val;
}

inline void nas_qos_scheduler::set_min_burst(uint64_t val)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_PROFILE_MIN_BURST);
    cfg.min_burst = val;
}

inline void nas_qos_scheduler::set_max_rate(uint64_t val)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_PROFILE_MAX_RATE);
    cfg.max_rate = val;
}

inline void nas_qos_scheduler::set_max_burst(uint64_t val)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_PROFILE_MAX_BURST);
    cfg.max_burst = val;
}



#endif
