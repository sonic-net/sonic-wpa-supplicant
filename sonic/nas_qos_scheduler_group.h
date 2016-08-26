
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
 * \file   nas_qos_scheduler_group.h
 * \brief  NAS QOS SCHEDULER_GROUP object
 */

#ifndef _NAS_QOS_SCHEDULER_GROUP_H_
#define _NAS_QOS_SCHEDULER_GROUP_H_

#include <unordered_map>


#include "nas_base_utils.h"
#include "nas_qos_common.h"
#include "std_type_defs.h"
#include "ds_common_types.h" // npu_id_t
#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_ndi_qos.h"
#include "nas_ndi_common.h"

class nas_qos_switch;

//@todo get MAX level from Switch attribute
#define MAX_SCHEDULER_LEVEL 4 // @FIXME

typedef struct nas_qos_scheduler_group_struct{
    uint32_t         max_child;
    hal_ifindex_t    port_id;
    uint32_t         level;
    nas_obj_id_t     scheduler_profile_id;
    std::vector<nas_obj_id_t> child_list;
} nas_qos_scheduler_group_struct_t;

class nas_qos_scheduler_group : public nas::base_obj_t
{

    // keys
    nas_obj_id_t scheduler_group_id;

    // parent
    nas_obj_id_t parent_id;

    // attributes
    nas_qos_scheduler_group_struct_t cfg;

    // cached info
    ndi_port_t   ndi_port_id; // derived from cfg.port_id, i.e. ifIndex

    ndi_obj_id_t ndi_scheduler_group_id;

    // These may be better for common utility routines.
    ndi_obj_id_t nas2ndi_scheduler_profile_id(nas_obj_id_t scheduler_profile_id,
                                                npu_id_t npu_id);
    bool nas2ndi_queue_id(nas_obj_id_t queue_id, ndi_obj_id_t &ndi_obj_id);
    bool nas2ndi_scheduler_group_id (nas_obj_id_t scheduler_group_id, ndi_obj_id_t &ndi_obj_id);
    ndi_obj_id_t nas2ndi_child_id(nas_obj_id_t child_id);
    t_std_error  get_max_child_from_ndi(npu_id_t npu_id);

    bool add_child_list_validate(npu_id_t npu_id, ndi_obj_id_t *id_list, uint id_count);
    bool del_child_list_validate(npu_id_t npu_id, ndi_obj_id_t *id_list, uint id_count);
    void update_child_list(npu_id_t npu_id, ndi_obj_id_t *id_list, uint id_count, bool add_child);

public:

    nas_qos_scheduler_group (nas_qos_switch* switch_p);

    const nas_qos_switch& get_switch() ;

    nas_obj_id_t get_scheduler_group_id() const {return scheduler_group_id;}
    void    set_scheduler_group_id(nas_obj_id_t id) {scheduler_group_id = id;}

    nas_obj_id_t get_scheduler_group_parent_id() {return parent_id;}
    void    set_scheduler_group_parent_id(nas_obj_id_t id) {parent_id = id;}
    bool    is_scheduler_group_attached() {
        return ((cfg.level == 0) || (parent_id != NDI_QOS_NULL_OBJECT_ID));
    }
    bool    next_level_is_queue() {return (cfg.level >= (MAX_SCHEDULER_LEVEL - 2));}

    uint32_t    get_max_child()    {return cfg.max_child;}
    // READ ONLY, no set function

    hal_ifindex_t    get_port_id() const {return cfg.port_id;}
    t_std_error set_port_id(hal_ifindex_t idx);

    uint32_t        get_level() const {return cfg.level;}
    void set_level(uint32_t val);

    nas_obj_id_t    get_scheduler_profile_id() const {return cfg.scheduler_profile_id;}
    void set_scheduler_profile_id(nas_obj_id_t id);

    uint32_t         get_child_count() { return cfg.child_list.size();}

    nas_obj_id_t     get_child_id(uint32_t idx) {return cfg.child_list[idx];}

    t_std_error      add_child(nas_obj_id_t child_id);

    void             clear_child(void);


    /// Overriding base object virtual functions
    virtual const char* name () const override { return "QOS SCHEDULER_GROUP";}

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
    virtual void push_non_leaf_attr_ndi (nas_attr_id_t   non_leaf_attr_id,
                                         base_obj_t&   obj_old,
                                         nas::npu_set_t  npu_list,
                                         nas::rollback_trakr_t& r_trakr,
                                         bool rolling_back) override;
    virtual void rollback_non_leaf_attr_in_npu (const nas::attr_list_t& attr_hierarchy,
                                                npu_id_t npu_id,
                                                base_obj_t& obj_new) override;

    ndi_obj_id_t      ndi_obj_id (npu_id_t npu_id) const;
    void set_ndi_obj_id (npu_id_t npu_id,
                         ndi_obj_id_t obj_id);
    void reset_ndi_obj_id (npu_id_t npu_id);

    void modify_child_list(nas_qos_scheduler_group * old_sg, npu_id_t npu_id);

} ;

inline ndi_obj_id_t nas_qos_scheduler_group::ndi_obj_id (npu_id_t npu_id) const
{
    return (ndi_scheduler_group_id);
}

inline void nas_qos_scheduler_group::set_ndi_obj_id (npu_id_t npu_id,
                                           ndi_obj_id_t id)
{
    ndi_scheduler_group_id = id;
}

inline void nas_qos_scheduler_group::reset_ndi_obj_id (npu_id_t npu_id)
{
    ndi_scheduler_group_id = 0;
}

inline void nas_qos_scheduler_group::set_level(uint32_t val)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_GROUP_LEVEL);
    cfg.level = val;
}

inline void nas_qos_scheduler_group::set_scheduler_profile_id(nas_obj_id_t id)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID);
    cfg.scheduler_profile_id = id;
}

inline t_std_error nas_qos_scheduler_group::add_child(nas_obj_id_t child_id)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_GROUP_CHILD_LIST);
    cfg.child_list.push_back(child_id);

    return STD_ERR_OK;
}

inline void nas_qos_scheduler_group::clear_child(void)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_GROUP_CHILD_LIST);
    cfg.child_list.clear();
}

#endif
