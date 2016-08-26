
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
 * \file   nas_qos_switch.h
 * \brief  NAS QOS Switch Object
 */

#ifndef _NAS_QOS_SWITCH_H_
#define _NAS_QOS_SWITCH_H_

#include "nas_qos_common.h"
#include "nas_qos_policer.h"
#include "nas_qos_wred.h"
#include "nas_qos_queue.h"
#include "nas_qos_scheduler.h"
#include "nas_qos_scheduler_group.h"
#include "nas_qos_port_ingress.h"
#include "nas_qos_port_egress.h"
#include "nas_qos_map.h"
#include "nas_base_obj.h"
#include "nas_qos_buffer_pool.h"
#include "nas_qos_buffer_profile.h"
#include "nas_qos_priority_group.h"

#include <unordered_map>
#include <map>

typedef std::unordered_map<nas_obj_id_t, nas_qos_policer> policer_list_t;
typedef policer_list_t::iterator policer_iter_t;

typedef std::unordered_map<nas_obj_id_t, nas_qos_wred> wred_list_t;
typedef wred_list_t::iterator wred_iter_t;

typedef std::map<nas_qos_queue_key_t, nas_qos_queue> queue_list_t;
typedef queue_list_t::iterator queue_iter_t;

typedef std::unordered_map<nas_obj_id_t, nas_qos_scheduler> scheduler_list_t;
typedef scheduler_list_t::iterator scheduler_iter_t;

typedef std::unordered_map<nas_obj_id_t, nas_qos_scheduler_group> scheduler_group_list_t;
typedef scheduler_group_list_t::iterator scheduler_group_iter_t;

typedef std::unordered_map<nas_obj_id_t, nas_qos_map> map_list_t;
typedef map_list_t::iterator map_iter_t;

typedef std::map<hal_ifindex_t, nas_qos_port_ingress> port_ing_list_t;
typedef port_ing_list_t::iterator port_ing_iter_t;

typedef std::map<hal_ifindex_t, nas_qos_port_egress> port_egr_list_t;
typedef port_egr_list_t::iterator port_egr_iter_t;

typedef std::unordered_map<nas_obj_id_t, nas_qos_buffer_pool> buffer_pool_list_t;
typedef buffer_pool_list_t::iterator buffer_pool_iter_t;

typedef std::unordered_map<nas_obj_id_t, nas_qos_buffer_profile> buffer_profile_list_t;
typedef buffer_profile_list_t::iterator buffer_profile_iter_t;

typedef std::map<nas_qos_priority_group_key_t, nas_qos_priority_group> priority_group_list_t;
typedef priority_group_list_t::iterator priority_group_iter_t;

class nas_qos_switch : public nas::base_switch_t
{
    /************  Policers **********************************/
    policer_list_t     policers;

    static const size_t NAS_QOS_POLICER_ID_MAX = 500;

    nas::id_generator_t  _policer_id_gen {NAS_QOS_POLICER_ID_MAX};

    /************* WREDs *************************************/
    wred_list_t     wreds;

    static const size_t NAS_QOS_WRED_ID_MAX = 500;

    nas::id_generator_t  _wred_id_gen {NAS_QOS_WRED_ID_MAX};


    /************* Queues  ***********************************/
    queue_list_t           queues;

    static const size_t NAS_QOS_QUEUE_ID_MAX = 0xffff;

    nas::id_generator_t  _queue_id_gen {NAS_QOS_QUEUE_ID_MAX};


    /************  Schedulers ********************************/
    scheduler_list_t     schedulers;

    static const size_t NAS_QOS_SCHEDULER_ID_MAX = 0xffff;

    nas::id_generator_t  _scheduler_id_gen {NAS_QOS_SCHEDULER_ID_MAX};

    /************  Scheduler Groups ********************************/
    scheduler_group_list_t     scheduler_groups;

    static const size_t NAS_QOS_SCHEDULER_GROUP_ID_MAX = 0xffff;

    nas::id_generator_t  _scheduler_group_id_gen {NAS_QOS_SCHEDULER_GROUP_ID_MAX};

    /************* Maps *************************************/
    map_list_t     maps;

    static const size_t NAS_QOS_MAP_ID_MAX = 1000;

    nas::id_generator_t  _map_id_gen {NAS_QOS_MAP_ID_MAX};

    /************* Buffer Pools *************************************/
    buffer_pool_list_t     buffer_pools;

    static const size_t NAS_QOS_BUFFER_POOL_ID_MAX = 32;

    nas::id_generator_t  _buffer_pool_id_gen {NAS_QOS_BUFFER_POOL_ID_MAX};

    /************* Buffer Profiles *************************************/
    buffer_profile_list_t     buffer_profiles;

    static const size_t NAS_QOS_BUFFER_PROFILE_ID_MAX = 256;

    nas::id_generator_t  _buffer_profile_id_gen {NAS_QOS_BUFFER_PROFILE_ID_MAX};

    /************* Priority Groups *************************************/
    priority_group_list_t     priority_groups;

    static const size_t NAS_QOS_PRIORITY_GROUP_ID_MAX = 0x7FFF;

    nas::id_generator_t  _priority_group_id_gen {NAS_QOS_PRIORITY_GROUP_ID_MAX};

    /************* Port Ingress *************************************/
    port_ing_list_t port_ings;

    /************* Port Egress *************************************/
    port_egr_list_t port_egrs;

public:

    /* Default Constructor & Destructor */
    nas_qos_switch (nas_obj_id_t id): base_switch_t(id) { };

    // switch wide info
    uint_t ucast_queues_per_port;
    uint_t mcast_queues_per_port;
    uint_t total_queues_per_port;
    uint_t cpu_queues;

    /************** Policers ***************/

    t_std_error     add_policer (nas_qos_policer &t);

    void            remove_policer (nas_obj_id_t policer_id);

    nas_qos_policer* get_policer (nas_obj_id_t policer_id);

    nas_obj_id_t alloc_policer_id () {return _policer_id_gen.alloc_id ();}

    void release_policer_id(nas_obj_id_t id) {_policer_id_gen.release_id(id);}

    policer_iter_t get_policer_it_begin() {return policers.begin();}
    policer_iter_t get_policer_it_end()   {return policers.end();}


    /************* WRED *******************/
    t_std_error     add_wred (nas_qos_wred &t);

    void            remove_wred (nas_obj_id_t wred_id);

    nas_qos_wred* get_wred (nas_obj_id_t wred_id);

    nas_obj_id_t alloc_wred_id () {return _wred_id_gen.alloc_id ();}

    void release_wred_id(nas_obj_id_t id) {_wred_id_gen.release_id(id);}

    wred_iter_t get_wred_it_begin() {return wreds.begin();}
    wred_iter_t get_wred_it_end()   {return wreds.end();}

    /***************** Queues **************/
    t_std_error     add_queue (nas_qos_queue &t);

    void            remove_queue (nas_qos_queue_key_t key);
    nas_qos_queue*  get_queue(nas_qos_queue_key_t key);
    nas_qos_queue*  get_queue_by_id(ndi_obj_id_t queue_id);

    nas_obj_id_t alloc_queue_id () {return (_queue_id_gen.alloc_id () | NAS_QUEUE_ID_TYPE_MASK);}

    void release_queue_id(nas_obj_id_t id) {_queue_id_gen.release_id((id & (~NAS_QUEUE_ID_TYPE_MASK)));}

    void            dump_all_queues();

    // return the actual number of queues filled
    uint_t    get_port_queues(hal_ifindex_t port_id, uint_t count, nas_qos_queue * q_list[]);
    uint_t    get_number_of_port_queues(hal_ifindex_t port_id);
    bool      port_queue_is_initialized(hal_ifindex_t port_id);


    /************* Schedulers *******************/
    t_std_error     add_scheduler (nas_qos_scheduler &t);

    void            remove_scheduler (nas_obj_id_t scheduler_id);

    nas_qos_scheduler* get_scheduler (nas_obj_id_t scheduler_id);

    nas_obj_id_t alloc_scheduler_id () {return _scheduler_id_gen.alloc_id ();}

    void release_scheduler_id(nas_obj_id_t id) {_scheduler_id_gen.release_id(id);}

    scheduler_iter_t get_scheduler_it_begin() {return schedulers.begin();}
    scheduler_iter_t get_scheduler_it_end()   {return schedulers.end();}

    /************* Scheduler Groups *******************/
    t_std_error     add_scheduler_group (nas_qos_scheduler_group &t);

    void            remove_scheduler_group (nas_obj_id_t scheduler_group_id);

    nas_qos_scheduler_group* get_scheduler_group (nas_obj_id_t scheduler_group_id);
    nas_qos_scheduler_group* get_scheduler_group_by_id(npu_id_t npu_id, ndi_obj_id_t ndi_sg_id);

    nas_obj_id_t alloc_scheduler_group_id () {return _scheduler_group_id_gen.alloc_id () | NAS_SCHEDULER_GROUP_ID_TYPE_MASK;}

    void release_scheduler_group_id(nas_obj_id_t id) {_scheduler_group_id_gen.release_id((id & (~NAS_SCHEDULER_GROUP_ID_TYPE_MASK)));}

    t_std_error     get_port_scheduler_groups(hal_ifindex_t port_id, int match_level,
                                              std::vector<nas_qos_scheduler_group *>& sg_list);
    bool      port_sg_is_initialized(hal_ifindex_t port_id);

    scheduler_group_iter_t get_scheduler_group_it_begin() {return scheduler_groups.begin();}
    scheduler_group_iter_t get_scheduler_group_it_end()   {return scheduler_groups.end();}

    /************* Map *******************/
    t_std_error     add_map (nas_qos_map &t);

    void            remove_map (nas_obj_id_t map_id);

    nas_qos_map* get_map (nas_obj_id_t map_id);

    nas_obj_id_t alloc_map_id () {return _map_id_gen.alloc_id ();}

    void release_map_id(nas_obj_id_t id) {_map_id_gen.release_id(id);}

    map_iter_t get_map_it_begin() {return maps.begin();}
    map_iter_t get_map_it_end()   {return maps.end();}

    /************* Buffer Pool *******************/
    t_std_error     add_buffer_pool (nas_qos_buffer_pool &t);

    void            remove_buffer_pool (nas_obj_id_t buffer_pool_id);

    nas_qos_buffer_pool* get_buffer_pool (nas_obj_id_t buffer_pool_id);

    nas_obj_id_t alloc_buffer_pool_id () {return _buffer_pool_id_gen.alloc_id ();}

    void release_buffer_pool_id(nas_obj_id_t id) {_buffer_pool_id_gen.release_id(id);}

    buffer_pool_iter_t get_buffer_pool_it_begin() {return buffer_pools.begin();}
    buffer_pool_iter_t get_buffer_pool_it_end()   {return buffer_pools.end();}

    /************* Buffer Profile *******************/
    t_std_error     add_buffer_profile (nas_qos_buffer_profile &t);

    void            remove_buffer_profile (nas_obj_id_t buffer_profile_id);

    nas_qos_buffer_profile* get_buffer_profile (nas_obj_id_t buffer_profile_id);

    nas_obj_id_t alloc_buffer_profile_id () {return _buffer_profile_id_gen.alloc_id ();}

    void release_buffer_profile_id(nas_obj_id_t id) {_buffer_profile_id_gen.release_id(id);}

    buffer_profile_iter_t get_buffer_profile_it_begin() {return buffer_profiles.begin();}
    buffer_profile_iter_t get_buffer_profile_it_end()   {return buffer_profiles.end();}

    /***************** Priority Groups **************/
    t_std_error     add_priority_group (nas_qos_priority_group &t);

    void            remove_priority_group (nas_qos_priority_group_key_t key);
    nas_qos_priority_group*  get_priority_group(nas_qos_priority_group_key_t key);
    nas_qos_priority_group*  get_priority_group_by_id(ndi_obj_id_t priority_group_id);

    nas_obj_id_t alloc_priority_group_id () {return (_priority_group_id_gen.alloc_id ());}

    void release_priority_group_id(nas_obj_id_t id) {_priority_group_id_gen.release_id(id);}

    void            dump_all_priority_groups();

    // return the actual number of priority_groups filled
    uint_t    get_port_priority_groups(hal_ifindex_t port_id, uint_t count, nas_qos_priority_group * q_list[]);
    uint_t    get_number_of_port_priority_groups(hal_ifindex_t port_id);
    bool      port_priority_group_is_initialized(hal_ifindex_t port_id);


    /************* Port Ingress *******************/
    t_std_error             add_port_ingress(nas_qos_port_ingress& t);

    void                    remove_port_ingress(hal_ifindex_t port_id);

    nas_qos_port_ingress*   get_port_ingress(hal_ifindex_t port_id);

    void                    dump_all_port_ing_profile();

    bool                    port_ing_is_initialized(hal_ifindex_t port_id);

    /************* Port Egress *******************/
    t_std_error             add_port_egress(nas_qos_port_egress& t);

    void                    remove_port_egress(hal_ifindex_t port_id);

    nas_qos_port_egress*    get_port_egress(hal_ifindex_t port_id);

    void                    dump_all_port_egr_profile();

    bool                    port_egr_is_initialized(hal_ifindex_t port_id);
};
#endif
