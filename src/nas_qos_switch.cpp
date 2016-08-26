
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
 * \file   nas_qos_switch.cpp
 * \brief  NAS QOS Switch Object
 */

#include "std_assert.h"
#include "event_log.h"
#include "nas_qos_common.h"
#include "nas_qos_switch.h"

/************************** Policers *******************/
nas_qos_policer * nas_qos_switch::get_policer(nas_obj_id_t policer_id)
{
    policer_iter_t pi = policers.find(policer_id);
    return ((pi != policers.end())? &(pi->second): NULL);
}


t_std_error nas_qos_switch::add_policer (nas_qos_policer& p)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_policer(p.policer_id())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "Policer_id exists: %d", p.policer_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "Policer Exists"};
    }

    policers.insert(std::make_pair(p.policer_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_policer (nas_obj_id_t policer_id)
{
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING Policer_id in switch: %d", policer_id);
    release_policer_id(policer_id);

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING Policer_id from Policer List: %d", policer_id);
    policers.erase(policer_id);

    return;
}

/***************** WRED *************************/

nas_qos_wred * nas_qos_switch::get_wred(nas_obj_id_t wred_id)
{
    wred_iter_t pi = wreds.find(wred_id);
    return ((pi != wreds.end())? &(pi->second): NULL);
}


t_std_error nas_qos_switch::add_wred (nas_qos_wred& p)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_wred(p.get_wred_id())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "wred_id exists: %d", p.get_wred_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "wred Exists"};
    }

    wreds.insert(std::make_pair(p.get_wred_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_wred (nas_obj_id_t wred_id)
{
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING wred_id in switch: %d", wred_id);
    release_wred_id(wred_id);

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING wred_id from wred List: %d", wred_id);
    wreds.erase(wred_id);

    return;
}


/************************ Queues *******************/

t_std_error nas_qos_switch::add_queue(nas_qos_queue &q)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_queue(q.get_key())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "Queue with the same key exists");
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "Queue Exists"};
    }

    queues.insert(std::make_pair(q.get_key(), std::move(q)));

    return STD_ERR_OK;

}

void nas_qos_switch::remove_queue(nas_qos_queue_key_t key)
{

    nas_qos_queue * q = get_queue(key);
    if (q == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "Key not found ");
        return;
    }

    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING queue_id in switch: %d",
                q->get_queue_id());
    release_queue_id(q->get_queue_id());

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING queue from queue List");
    queues.erase(key);

    return;
}

nas_qos_queue * nas_qos_switch::get_queue(nas_qos_queue_key_t key)
{
    queue_iter_t qi = queues.find(key);
    return ((qi != queues.end())? &(qi->second): NULL);
}

nas_qos_queue * nas_qos_switch::get_queue_by_id(ndi_obj_id_t queue_id)
{
    for (auto& queue_info: queues) {
        if (queue_info.second.ndi_obj_id() == queue_id) {
            return &queue_info.second;
        }
    }
    return NULL;
}

void nas_qos_switch::dump_all_queues()
{
    for (auto queue:  queues) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                "nas: (port_id %u, local queue_id %u, type %u): queue_id 0x%ul ",
                queue.first.port_id,
                (uint_t)queue.first.local_queue_id,
                (uint_t)queue.first.type,
                queue.second.get_queue_id());
        for (auto npu_id: queue.second.npu_list()) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                        "    ndi: npu %u, ndi_queue_id %u  ",
                        npu_id, (uint_t)queue.second.ndi_obj_id());
        }
    }
}

uint_t nas_qos_switch::get_port_queues(hal_ifindex_t port_id,
        uint_t count, nas_qos_queue *q_list[])
{
    queue_iter_t it;
    nas_qos_queue_key_t key = {0};
    key.port_id = port_id;
    uint_t i = 0;

    it = queues.lower_bound(key);
    for (; it != queues.end(); it++) {
        if (it->second.get_key().port_id > port_id)
            break;

        if (i < count)
            q_list[i++] = &(it->second);
    }
    return i;
}

uint_t    nas_qos_switch::get_number_of_port_queues(hal_ifindex_t port_id)
{
    queue_iter_t it;
    nas_qos_queue_key_t key = {0};
    key.port_id = port_id;
    uint_t count = 0;

    it = queues.lower_bound(key);
    for (; it != queues.end(); it++) {
        if (it->second.get_key().port_id > port_id)
            break;

        count++;
    }

    return count;
}

bool    nas_qos_switch::port_queue_is_initialized(hal_ifindex_t port_id)
{
    queue_iter_t it;
    nas_qos_queue_key_t key = {0};
    key.port_id = port_id;

    it = queues.lower_bound(key);
    if (it == queues.end())
        return false;

    if (it->second.get_key().port_id > port_id)
        return false;

    // matching port-id
    return true;
}



/******************* Schedulers *******************/


nas_qos_scheduler * nas_qos_switch::get_scheduler(nas_obj_id_t scheduler_id)
{
    scheduler_iter_t pi = schedulers.find(scheduler_id);
    return ((pi != schedulers.end())? &(pi->second): NULL);
}


t_std_error nas_qos_switch::add_scheduler (nas_qos_scheduler& p)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_scheduler(p.get_scheduler_id())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "scheduler_id exists: %d", p.get_scheduler_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "scheduler Exists"};
    }

    schedulers.insert(std::make_pair(p.get_scheduler_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_scheduler (nas_obj_id_t scheduler_id)
{
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING scheduler_id in switch: %d", scheduler_id);
    release_scheduler_id(scheduler_id);

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING scheduler_id from scheduler List: %d", scheduler_id);
    schedulers.erase(scheduler_id);

    return;
}

/******************* Scheduler Groups *******************/


nas_qos_scheduler_group * nas_qos_switch::get_scheduler_group(nas_obj_id_t scheduler_group_id)
{
    scheduler_group_iter_t pi = scheduler_groups.find(scheduler_group_id);
    return ((pi != scheduler_groups.end())? &(pi->second): NULL);
}

nas_qos_scheduler_group * nas_qos_switch::get_scheduler_group_by_id(npu_id_t npu_id,
                                                                    nas_obj_id_t ndi_sg_id)
{
    for (auto& sg_info: scheduler_groups) {
        if (sg_info.second.ndi_obj_id(npu_id) == ndi_sg_id) {
            return &sg_info.second;
        }
    }
    return NULL;
}

t_std_error nas_qos_switch::get_port_scheduler_groups(hal_ifindex_t port_id, int match_level,
                                                std::vector<nas_qos_scheduler_group *>& sg_list)
{
    hal_ifindex_t port;
    uint32_t level;
    for (auto& it : scheduler_groups) {
        port = it.second.get_port_id();
        level = it.second.get_level();
        if (port != port_id) {
            continue;
        }
        if (match_level < 0 || (uint32_t)match_level == level) {
            sg_list.push_back(&(it.second));
        }
    }

    return STD_ERR_OK;
}

t_std_error nas_qos_switch::add_scheduler_group (nas_qos_scheduler_group& p)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_scheduler_group(p.get_scheduler_group_id())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "scheduler_group_id exists: %d", p.get_scheduler_group_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "scheduler_group Exists"};
    }

    scheduler_groups.insert(std::make_pair(p.get_scheduler_group_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_scheduler_group (nas_obj_id_t scheduler_group_id)
{
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING scheduler_group_id in switch: %d", scheduler_group_id);
    release_scheduler_group_id(scheduler_group_id);

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING scheduler_group_id from scheduler_group List: %d", scheduler_group_id);
    scheduler_groups.erase(scheduler_group_id);

    return;
}

bool nas_qos_switch::port_sg_is_initialized(hal_ifindex_t port_id)
{
    hal_ifindex_t port;
    for (auto& it: scheduler_groups) {
        port = it.second.get_port_id();
        if (port == port_id) {
            return true;
        }
    }

    return false;
}

/***************** Buffer Profile *************************/

nas_qos_buffer_profile * nas_qos_switch::get_buffer_profile(nas_obj_id_t buffer_profile_id)
{
    buffer_profile_iter_t pi = buffer_profiles.find(buffer_profile_id);
    return ((pi != buffer_profiles.end())? &(pi->second): NULL);
}


t_std_error nas_qos_switch::add_buffer_profile (nas_qos_buffer_profile& p)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_buffer_profile(p.get_buffer_profile_id())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "buffer_profile_id exists: %d", p.get_buffer_profile_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "buffer_profile Exists"};
    }

    buffer_profiles.insert(std::make_pair(p.get_buffer_profile_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_buffer_profile (nas_obj_id_t buffer_profile_id)
{
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING buffer_profile_id in switch: %d", buffer_profile_id);
    release_buffer_profile_id(buffer_profile_id);

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING buffer_profile_id from buffer_profile List: %d", buffer_profile_id);
    buffer_profiles.erase(buffer_profile_id);

    return;
}

/***************** Buffer Pool *************************/

nas_qos_buffer_pool * nas_qos_switch::get_buffer_pool(nas_obj_id_t buffer_pool_id)
{
    buffer_pool_iter_t pi = buffer_pools.find(buffer_pool_id);
    return ((pi != buffer_pools.end())? &(pi->second): NULL);
}


t_std_error nas_qos_switch::add_buffer_pool (nas_qos_buffer_pool& p)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_buffer_pool(p.get_buffer_pool_id())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "buffer_pool_id exists: %d", p.get_buffer_pool_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "buffer_pool Exists"};
    }

    buffer_pools.insert(std::make_pair(p.get_buffer_pool_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_buffer_pool (nas_obj_id_t buffer_pool_id)
{
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING buffer_pool_id in switch: %d", buffer_pool_id);
    release_buffer_pool_id(buffer_pool_id);

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING buffer_pool_id from buffer_pool List: %d", buffer_pool_id);
    buffer_pools.erase(buffer_pool_id);

    return;
}


/************************ Priority Groups *******************/

t_std_error nas_qos_switch::add_priority_group(nas_qos_priority_group &q)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_priority_group(q.get_key())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "priority_group with the same key exists");
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "priority_group Exists"};
    }

    priority_groups.insert(std::make_pair(q.get_key(), std::move(q)));

    return STD_ERR_OK;

}

void nas_qos_switch::remove_priority_group(nas_qos_priority_group_key_t key)
{

    nas_qos_priority_group * q = get_priority_group(key);
    if (q == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "Key not found ");
        return;
    }

    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING priority_group_id in switch: %d",
                q->get_priority_group_id());
    release_priority_group_id(q->get_priority_group_id());

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING priority_group from priority_group List");
    priority_groups.erase(key);

    return;
}

nas_qos_priority_group * nas_qos_switch::get_priority_group(nas_qos_priority_group_key_t key)
{
    priority_group_iter_t qi = priority_groups.find(key);
    return ((qi != priority_groups.end())? &(qi->second): NULL);
}

nas_qos_priority_group * nas_qos_switch::get_priority_group_by_id(ndi_obj_id_t priority_group_id)
{
    for (auto& priority_group_info: priority_groups) {
        if (priority_group_info.second.ndi_obj_id() == priority_group_id) {
            return &priority_group_info.second;
        }
    }
    return NULL;
}

void nas_qos_switch::dump_all_priority_groups()
{
    for (auto priority_group:  priority_groups) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                "nas: (port_id %u, local id %u): priority_group_id 0x%ul ",
                priority_group.first.port_id,
                (uint_t)priority_group.first.local_id,
                priority_group.second.get_priority_group_id());
        for (auto npu_id: priority_group.second.npu_list()) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                        "    ndi: npu %u, ndi_priority_group_id %u  ",
                        npu_id, (uint_t)priority_group.second.ndi_obj_id());
        }
    }
}

uint_t nas_qos_switch::get_port_priority_groups(hal_ifindex_t port_id,
        uint_t count, nas_qos_priority_group *q_list[])
{
    priority_group_iter_t it;
    nas_qos_priority_group_key_t key = {0};
    key.port_id = port_id;
    uint_t i = 0;

    it = priority_groups.lower_bound(key);
    for (; it != priority_groups.end(); it++) {
        if (it->second.get_key().port_id > port_id)
            break;

        if (i < count)
            q_list[i++] = &(it->second);
    }
    return i;
}

uint_t    nas_qos_switch::get_number_of_port_priority_groups(hal_ifindex_t port_id)
{
    priority_group_iter_t it;
    nas_qos_priority_group_key_t key = {0};
    key.port_id = port_id;
    uint_t count = 0;

    it = priority_groups.lower_bound(key);
    for (; it != priority_groups.end(); it++) {
        if (it->second.get_key().port_id > port_id)
            break;

        count++;
    }

    return count;
}

bool    nas_qos_switch::port_priority_group_is_initialized(hal_ifindex_t port_id)
{
    priority_group_iter_t it;
    nas_qos_priority_group_key_t key = {0};
    key.port_id = port_id;

    it = priority_groups.lower_bound(key);
    if (it == priority_groups.end())
        return false;

    if (it->second.get_key().port_id > port_id)
        return false;

    // matching port-id
    return true;
}



/***************** Maps *************************/

nas_qos_map * nas_qos_switch::get_map(nas_obj_id_t map_id)
{
    map_iter_t pi = maps.find(map_id);
    return ((pi != maps.end())? &(pi->second): NULL);
}


t_std_error nas_qos_switch::add_map (nas_qos_map& p)
{
    /* Do NOT allow overwrite of existing entry */
    if (get_map(p.get_map_id())) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "map_id exists: %d", p.get_map_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__, "map Exists"};
    }

    maps.insert(std::make_pair(p.get_map_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_map (nas_obj_id_t map_id)
{
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING map_id in switch: %d", map_id);
    release_map_id(map_id);

    // remove from switch
    EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "FREEING map_id from map List: %d", map_id);
    maps.erase(map_id);

    return;
}

/***************** Port Ingress *************************/

nas_qos_port_ingress* nas_qos_switch::get_port_ingress(hal_ifindex_t ifindex)
{
    port_ing_iter_t pi = port_ings.find(ifindex);
    return (pi != port_ings.end() ? &((*pi).second) : NULL);
}

t_std_error nas_qos_switch::add_port_ingress(nas_qos_port_ingress& p)
{
    if (get_port_ingress(p.get_port_id()) != NULL) {
        EV_LOG_ERR(ev_log_t_QOS, ev_log_s_WARNING, "NAS-QOS",
                "Port ingress profile for port %d alreay exists\n",
                p.get_port_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__,
                "port ingress Exists"};
    }
    port_ings.insert(std::make_pair(p.get_port_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_port_ingress(hal_ifindex_t ifindex)
{
    port_ings.erase(ifindex);
}

void nas_qos_switch::dump_all_port_ing_profile()
{
    for (auto port_ing: port_ings) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                "nas: port_ingress (port_id %u, default_tc %u)",
                port_ing.first,
                port_ing.second.get_default_traffic_class());
    }
}

bool nas_qos_switch::port_ing_is_initialized(hal_ifindex_t port_id)
{
    port_ing_iter_t it = port_ings.find(port_id);
    return (it != port_ings.end());
}

/***************** Port Egress *************************/

nas_qos_port_egress* nas_qos_switch::get_port_egress(hal_ifindex_t ifindex)
{
    port_egr_iter_t pi = port_egrs.find(ifindex);
    return (pi != port_egrs.end() ? &((*pi).second) : NULL);
}

t_std_error nas_qos_switch::add_port_egress(nas_qos_port_egress& p)
{
    if (get_port_egress(p.get_port_id()) != NULL) {
        EV_LOG_ERR(ev_log_t_QOS, ev_log_s_WARNING, "NAS-QOS",
                "Port egress profile for port %d alreay exists\n",
                p.get_port_id());
        throw nas::base_exception {NAS_BASE_E_DUPLICATE, __PRETTY_FUNCTION__,
                "port egress Exists"};
    }
    port_egrs.insert(std::make_pair(p.get_port_id(), std::move(p)));

    return STD_ERR_OK;
}

void nas_qos_switch::remove_port_egress(hal_ifindex_t ifindex)
{
    port_egrs.erase(ifindex);
}

void nas_qos_switch::dump_all_port_egr_profile()
{
    for (auto port_eg: port_egrs) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MINOR, "QOS",
                "nas: port_egress (port_id %u, scheduler_profile_id 0x%016lx)",
                port_eg.first,
                port_eg.second.get_scheduler_profile_id());
    }
}

bool nas_qos_switch::port_egr_is_initialized(hal_ifindex_t port_id)
{
    port_egr_iter_t it = port_egrs.find(port_id);
    return (it != port_egrs.end());
}
