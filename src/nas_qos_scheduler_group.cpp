
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
 * \file   nas_qos_scheduler_group.cpp
 * \brief  NAS QOS SCHEDULER_GROUP Object
 */

#include "event_log.h"
#include "std_assert.h"
#include "nas_qos_common.h"
#include "nas_qos_scheduler_group.h"
#include "dell-base-qos.h"
#include "nas_ndi_qos.h"
#include "nas_base_obj.h"
#include "nas_qos_switch.h"
#include "nas_ndi_qos.h"
#include "hal_if_mapping.h"

nas_qos_scheduler_group::nas_qos_scheduler_group (nas_qos_switch* switch_p)
           : base_obj_t (switch_p)
{
    memset(&cfg, 0, sizeof(cfg));
    parent_id = NDI_QOS_NULL_OBJECT_ID;
    cfg.child_list = std::vector<nas_obj_id_t>();
    ndi_port_id = {0};
    scheduler_group_id = 0;
    ndi_scheduler_group_id = 0;
}

const nas_qos_switch& nas_qos_scheduler_group::get_switch()
{
    return static_cast<const nas_qos_switch&> (base_obj_t::get_switch());
}


void nas_qos_scheduler_group::commit_create (bool rolling_back)

{
    if ((!is_attr_dirty (BASE_QOS_SCHEDULER_GROUP_PORT_ID)) ||
        (!is_attr_dirty (BASE_QOS_SCHEDULER_GROUP_LEVEL)))    {
        throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                        __PRETTY_FUNCTION__,
                        "Mandatory attribute Port ID or Level not present"};
    }

    base_obj_t::commit_create(rolling_back);
}

void* nas_qos_scheduler_group::alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m)
{
    // NAS Qos scheduler_group does not allocate memory to save the incoming tentative attributes
    return this;
}

bool nas_qos_scheduler_group::push_create_obj_to_npu (npu_id_t npu_id,
                                     void* ndi_obj)
{
    ndi_obj_id_t ndi_scheduler_group_id;
    t_std_error rc;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Creating obj on NPU %d", npu_id);

    nas_qos_scheduler_group * nas_qos_scheduler_group_p = static_cast<nas_qos_scheduler_group*> (ndi_obj);

    // form attr_list
    std::vector<uint64_t> attr_list;
    attr_list.resize(_set_attributes.len());

    uint_t num_attr = 0;
    for (auto attr_id: _set_attributes) {
        attr_list[num_attr++] = attr_id;
    }

    ndi_qos_scheduler_group_struct_t  ndi_sg = {0};
    std::vector<ndi_obj_id_t> child_list;
    ndi_sg.level = nas_qos_scheduler_group_p->cfg.level;
    ndi_sg.ndi_port = nas_qos_scheduler_group_p->ndi_port_id;
    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Creating obj on NPU %d, npu_id %u, port_id %u", npu_id, ndi_sg.ndi_port.npu_id, ndi_sg.ndi_port.npu_port);

    std::vector<uint64_t>::iterator it;
    it = find(attr_list.begin(), attr_list.end(), BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID);
    if (it != attr_list.end()) {
        ndi_sg.scheduler_profile_id = nas2ndi_scheduler_profile_id(nas_qos_scheduler_group_p->cfg.scheduler_profile_id, npu_id);
    }
    ndi_sg.child_count = nas_qos_scheduler_group_p->cfg.child_list.size();
    if (ndi_sg.child_count) {
        child_list.resize(ndi_sg.child_count);
        for (uint_t i = 0; i< ndi_sg.child_count; i++) {
            child_list[i] = nas2ndi_child_id(nas_qos_scheduler_group_p->cfg.child_list[i]);
        }
        ndi_sg.child_list = &child_list[0];
    }
    else
        ndi_sg.child_list = nullptr;

    if ((rc = ndi_qos_create_scheduler_group (npu_id,
                                   &attr_list[0],
                                   num_attr,
                                   &ndi_sg,
                                   &ndi_scheduler_group_id))
            != STD_ERR_OK)
    {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Creating scheduler group on NPU %d failed!", npu_id);
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI QoS SCHEDULER_GROUP Create Failed"};
    }
    // Cache the new SCHEDULER_GROUP ID generated by NDI
    set_ndi_obj_id(npu_id, ndi_scheduler_group_id);

    return true;

}


bool nas_qos_scheduler_group::push_delete_obj_to_npu (npu_id_t npu_id)
{
    t_std_error rc;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Deleting obj on NPU %d", npu_id);

    if ((rc = ndi_qos_delete_scheduler_group(npu_id, ndi_obj_id(npu_id)))
        != STD_ERR_OK)
    {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI SCHEDULER_GROUP Delete Failed"};
    }

    return true;
}

bool nas_qos_scheduler_group::is_leaf_attr (nas_attr_id_t attr_id)
{
    // Table of function pointers to handle modify of Qos scheduler_group
    // attributes.
    static const std::unordered_map <BASE_QOS_SCHEDULER_GROUP_t,
                                     bool,
                                     std::hash<int>>
        _leaf_attr_map =
    {
        // modifiable objects
        {BASE_QOS_SCHEDULER_GROUP_CHILD_COUNT,  true},
        {BASE_QOS_SCHEDULER_GROUP_CHILD_LIST,   false},
        {BASE_QOS_SCHEDULER_GROUP_PORT_ID,      true},
        {BASE_QOS_SCHEDULER_GROUP_LEVEL,        true},
        {BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID, true},
    };

    return (_leaf_attr_map.at(static_cast<BASE_QOS_SCHEDULER_GROUP_t>(attr_id)));
}

bool nas_qos_scheduler_group::push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                           npu_id_t npu_id)
{
    t_std_error rc = STD_ERR_OK;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Modifying npu: %d, attr_id %d",
                    npu_id, attr_id);

    ndi_qos_scheduler_group_struct_t ndi_sg = {0};

    switch (attr_id) {
    case BASE_QOS_SCHEDULER_GROUP_PORT_ID:
        ndi_sg.ndi_port = ndi_port_id;
        break;

    case BASE_QOS_SCHEDULER_GROUP_LEVEL:
        ndi_sg.level = get_level();
        break;

    case BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID:
        ndi_sg.scheduler_profile_id = nas2ndi_scheduler_profile_id(get_scheduler_profile_id(), npu_id);
        break;

    case BASE_QOS_SCHEDULER_GROUP_CHILD_COUNT:
        ndi_sg.child_count = get_child_count();
        break;

    case BASE_QOS_SCHEDULER_GROUP_CHILD_LIST: // Leaf-list object
    default:
            STD_ASSERT (0); //non-modifiable object
    }

    rc = ndi_qos_set_scheduler_group_attr(npu_id,
                                   ndi_obj_id(npu_id),
                                   (BASE_QOS_SCHEDULER_GROUP_t)attr_id,
                                   &ndi_sg);
    if (rc != STD_ERR_OK) {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI attribute Set Failed"};
    }

    return true;
}

ndi_obj_id_t nas_qos_scheduler_group::nas2ndi_scheduler_profile_id(nas_obj_id_t scheduler_id, npu_id_t npu_id)
{
    if (scheduler_id == 0LL) {
        return NDI_QOS_NULL_OBJECT_ID;
    }

    nas_qos_switch & p_switch = const_cast <nas_qos_switch &> (get_switch());

    nas_qos_scheduler* p_scheduler = p_switch.get_scheduler(scheduler_id);

    if (p_scheduler == NULL) {
        t_std_error rc = STD_ERR(QOS, FAIL, 0);
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "Scheduler Profile id is not created in ndi yet"};
    }

    return p_scheduler->ndi_obj_id(npu_id);
}

bool nas_qos_scheduler_group::nas2ndi_scheduler_group_id(nas_obj_id_t scheduler_group_id, ndi_obj_id_t &ndi_obj_id)
{
    nas_qos_switch& p_switch = const_cast <nas_qos_switch &> (get_switch());

    nas_qos_scheduler_group* p = p_switch.get_scheduler_group(scheduler_group_id);

    if (p == NULL) {
        return false;
    }

    ndi_obj_id = p->ndi_scheduler_group_id;
    return true;
}

bool nas_qos_scheduler_group::nas2ndi_queue_id(nas_obj_id_t queue_id, ndi_obj_id_t &ndi_obj_id)
{
    nas_qos_switch& p_switch = const_cast <nas_qos_switch &> (get_switch());

    uint_t count = p_switch.get_number_of_port_queues(this->cfg.port_id);
    if (count == 0){
        return false;
    }

    std::vector<nas_qos_queue *> q_list(count);
    p_switch.get_port_queues(this->cfg.port_id, count, &q_list[0]);

    for (uint_t i = 0; i< count; i++ ) {
        nas_qos_queue *p = q_list[i];
        if (p->get_queue_id() == queue_id) {
            ndi_obj_id = p->ndi_obj_id();
            return true;
        }
    }

    return false; //not found
}

ndi_obj_id_t nas_qos_scheduler_group::nas2ndi_child_id(nas_obj_id_t child_id)
{
    t_std_error rc = STD_ERR(QOS, FAIL, 0);
    ndi_obj_id_t ndi_obj_id;

    if (get_level() >= (MAX_SCHEDULER_LEVEL - 1)) {
        // myself is at queue level, no more child
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "Queue node cannot have child nodes"};
    }
    else if (this->get_level() == (MAX_SCHEDULER_LEVEL - 2)) {
        // search only queue
        if (nas2ndi_queue_id(child_id, ndi_obj_id) == true)
            return ndi_obj_id;
    }
    else {
        // search both queue and Scheduler group
        if (nas2ndi_scheduler_group_id(child_id, ndi_obj_id) == true)
            return ndi_obj_id;
        else if (nas2ndi_queue_id(child_id, ndi_obj_id) == true)
            return ndi_obj_id;
    }
    throw nas::base_exception {rc, __PRETTY_FUNCTION__,
        "Child id does not exist in NAS"};

}

void nas_qos_scheduler_group::push_non_leaf_attr_ndi (nas_attr_id_t attr_id,
                                     base_obj_t&   obj_old,
                                     nas::npu_set_t  npu_list,
                                     nas::rollback_trakr_t& r_trakr,
                                     bool rolling_back)
{

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Modifying non-leaf attr_id %d",
                    attr_id);

    for (auto npu_id: npu_list) {
        switch (attr_id) {

        case BASE_QOS_SCHEDULER_GROUP_CHILD_LIST:

            modify_child_list((nas_qos_scheduler_group *) &obj_old, npu_id);
            break;

        default:
                STD_ASSERT (0); //leaf object or non-modifiable object
        }
    }

    return;

}

bool  nas_qos_scheduler_group::add_child_list_validate(npu_id_t npu_id,
                                        ndi_obj_id_t *id_list, uint id_count)
{
    uint idx;

    if (!is_scheduler_group_attached()) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                     "Couldn't add child to un-attached scheduler-group");
        return false;
    }

    nas_qos_switch& p_switch = const_cast<nas_qos_switch&>(get_switch());
    for (idx = 0; idx < id_count; idx ++) {
        if (next_level_is_queue()) {
            nas_qos_queue *p_queue = p_switch.get_queue_by_id(id_list[idx]);
            if (!p_queue || p_queue->is_queue_attached()) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                             "Queue %lld not exist or already attached", id_list[idx]);
                return false;
            }
        } else {
            nas_qos_scheduler_group *p_sg = p_switch.get_scheduler_group_by_id(npu_id,
                                                                               id_list[idx]);
            if (!p_sg || p_sg->is_scheduler_group_attached() ||
                p_sg->get_level() != cfg.level + 1) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                             "Scheduler-group %lld not exist or invalid to add", id_list[idx]);
                return false;
            }
        }
    }

    return true;
}

bool  nas_qos_scheduler_group::del_child_list_validate(npu_id_t npu_id,
                                            ndi_obj_id_t *id_list, uint id_count)
{
    uint idx;

    nas_qos_switch& p_switch = const_cast<nas_qos_switch&>(get_switch());
    for (idx = 0; idx < id_count; idx ++) {
        if (next_level_is_queue()) {
            nas_qos_queue *p_queue = p_switch.get_queue_by_id(id_list[idx]);
            if (!p_queue || p_queue->get_queue_parent_id() != scheduler_group_id) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                             "Queue %lld does not exist or is not child of this node",
                             id_list[idx]);
                return false;
            }
        } else {
            nas_qos_scheduler_group *p_sg = p_switch.get_scheduler_group_by_id(npu_id,
                                                                               id_list[idx]);
            if (!p_sg || p_sg->get_scheduler_group_parent_id() != scheduler_group_id) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                             "Scheduler-group %lld does not exist or is not child of this node",
                             id_list[idx]);
                return false;
            }
            if (p_sg->get_child_count() > 0) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                             "Scheduler-group %lld has children and couldn't be detached",
                             id_list[idx]);
                return false;
            }
        }
    }

    return true;
}

void  nas_qos_scheduler_group::update_child_list(npu_id_t npu_id,
                                        ndi_obj_id_t *id_list, uint id_count,
                                        bool add_child)
{
    uint idx;

    nas_qos_switch& p_switch = const_cast<nas_qos_switch&>(get_switch());
    ndi_obj_id_t parent_id = add_child ? scheduler_group_id : NDI_QOS_NULL_OBJECT_ID;
    for (idx = 0; idx < id_count; idx ++) {
        if (next_level_is_queue()) {
            nas_qos_queue *p_queue = p_switch.get_queue_by_id(id_list[idx]);
            if (!p_queue) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                             "Queue %lld does not exist",
                             id_list[idx]);
                continue;
            }
            p_queue->set_queue_parent_id(parent_id);
        } else {
            nas_qos_scheduler_group *p_sg = p_switch.get_scheduler_group_by_id(npu_id,
                                                                               id_list[idx]);
            if (!p_sg) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                             "Scheduler-group %lld not exist",
                             id_list[idx]);
                continue;
            }
            p_sg->set_scheduler_group_parent_id(parent_id);
        }
    }
}


void  nas_qos_scheduler_group::modify_child_list(nas_qos_scheduler_group *old_sg, npu_id_t npu_id)
{
    t_std_error rc = STD_ERR_OK;
    uint new_child_count = this->get_child_count();
    uint old_child_count = old_sg->get_child_count();

    std::vector<ndi_obj_id_t> add_list, del_list;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Entering modify_child_list, new_cc: %u old_cc: %u",
            new_child_count, old_child_count);

    // check new child list against old list
    uint add_count = 0;
    uint i, j;
    for (i = 0 ; i< new_child_count; i++) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Get child id idx %d", i);
        nas_obj_id_t new_child_id = this->get_child_id(i);
        EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "new_child_id 0x%016lX", i, new_child_id);

        for (j = 0; j< old_child_count; j++) {
            if (new_child_id == old_sg->get_child_id(j)) {
                break; //found
            }
        }

        if (j >= old_child_count) {
            // not found
            // add new child, child will be at current level plus 1
            add_count++;
            add_list.push_back(nas2ndi_child_id(new_child_id));
        }
    }

    // check old child list against new list
    uint del_count = 0;
    for (j = 0; j< old_child_count; j++) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Get old child id idx %d", j);
        nas_obj_id_t old_child_id = old_sg->get_child_id(j);
        EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Get old child id idx %d, done", j);

        for (i = 0; i< new_child_count; i++) {
            if (old_child_id == this->get_child_id(i)) {
                break; //found
            }
        }

        if (i >= new_child_count) {
            // not found
            // delete old child
            del_count++;
            del_list.push_back(nas2ndi_child_id(old_child_id));
        }
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "middle of modify_child_list");

    if (add_count) {
        if (!add_child_list_validate(npu_id, &add_list[0], add_count)) {
            throw nas::base_exception {(t_std_error)STD_ERR(QOS, CFG, 0),
                __PRETTY_FUNCTION__,
                "Child_list validation failed"};
        }
        rc = ndi_qos_add_child_to_scheduler_group(npu_id,
                                             ndi_scheduler_group_id,
                                             add_count,
                                             &add_list[0]);
        if (rc != STD_ERR_OK) {
            throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                "NDI attribute Child_list Set Failed"};
        }
        update_child_list(npu_id, &add_list[0], add_count, true);
    }

    if (del_count) {
        if (!del_child_list_validate(npu_id, &del_list[0], del_count)) {
            throw nas::base_exception {(t_std_error)STD_ERR(QOS, CFG, 0),
                __PRETTY_FUNCTION__,
                "Child_list validation failed"};
        }
        rc = ndi_qos_delete_child_from_scheduler_group(npu_id,
                                             ndi_scheduler_group_id,
                                             del_count,
                                             &del_list[0]);
        if (rc != STD_ERR_OK) {
            throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                "NDI attribute Child_list Set Failed"};
        }
        update_child_list(npu_id, &del_list[0], del_count, false);
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "existing modify_child_list");
}

void nas_qos_scheduler_group::rollback_non_leaf_attr_in_npu (const nas::attr_list_t& attr_hierarchy,
                                            npu_id_t npu_id,
                                            base_obj_t& obj_new)
{
    for (auto attr_id: attr_hierarchy) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Modifying non-leaf attr_id %d", attr_id);
        switch (attr_id) {
        case BASE_QOS_SCHEDULER_GROUP_CHILD_LIST:

            modify_child_list((nas_qos_scheduler_group *) &obj_new, npu_id);
            break;

        default:
            STD_ASSERT (0); //leaf object or non-modifiable object
        }
    }

    return;

}

t_std_error nas_qos_scheduler_group::set_port_id(hal_ifindex_t ifindex)
{
    mark_attr_dirty(BASE_QOS_SCHEDULER_GROUP_PORT_ID);
    cfg.port_id = ifindex;

    // populate the ndi-port cache
    interface_ctrl_t intf_ctrl;
    intf_ctrl.q_type = HAL_INTF_INFO_FROM_IF;
    intf_ctrl.if_index = ifindex;
    intf_ctrl.vrf_id = 0; //default vrf
    if (dn_hal_get_interface_info(&intf_ctrl) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Cannot find NPU id for ifIndex: %d",
                        ifindex);
        return STD_ERR(QOS, CFG, 0);
    }
    ndi_port_id.npu_id = intf_ctrl.npu_id;
    ndi_port_id.npu_port = intf_ctrl.port_id;

    return STD_ERR_OK;
}

