
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
 * \file   nas_qos_cps_buffer_pool.cpp
 * \brief  NAS qos buffer_pool related CPS API routines
 */

#include "cps_api_events.h"
#include "cps_api_operation.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"

#include "event_log_types.h"
#include "event_log.h"
#include "std_error_codes.h"
#include "std_mutex_lock.h"

#include "nas_qos_common.h"
#include "nas_qos_switch_list.h"
#include "nas_qos_cps.h"
#include "cps_api_key.h"
#include "dell-base-qos.h"
#include "nas_qos_buffer_pool.h"

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_buffer_pool &buffer_pool);
static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                const nas::attr_set_t attr_set,
                                                const nas_qos_buffer_pool &buffer_pool);
static nas_qos_buffer_pool * nas_qos_cps_get_buffer_pool(uint_t switch_id,
                                           nas_obj_id_t buffer_pool_id);
static cps_api_return_code_t nas_qos_cps_get_switch_and_buffer_pool_id(
                                    cps_api_object_t obj,
                                    uint_t &switch_id,
                                    nas_obj_id_t &buffer_pool_id);
static cps_api_return_code_t nas_qos_cps_api_buffer_pool_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_buffer_pool_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_buffer_pool_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t _append_one_buffer_pool(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_buffer_pool *buffer_pool);

static std_mutex_lock_create_static_init_rec(buffer_pool_mutex);

/**
  * This function provides NAS-QoS buffer_pool CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_pool_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&buffer_pool_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
        return nas_qos_cps_api_buffer_pool_create(obj, param->prev);

    case cps_api_oper_SET:
        return nas_qos_cps_api_buffer_pool_set(obj, param->prev);

    case cps_api_oper_DELETE:
        return nas_qos_cps_api_buffer_pool_delete(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}


/**
  * This function provides NAS-QoS buffer_pool CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_pool_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t buffer_pool_id_attr = cps_api_get_key_data(obj, BASE_QOS_BUFFER_POOL_ID);

    uint_t switch_id = 0;
    nas_obj_id_t buffer_pool_id = (buffer_pool_id_attr?
                                    cps_api_object_attr_data_u64(buffer_pool_id_attr): 0);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, buffer_pool id %u\n",
                    switch_id, buffer_pool_id);

    std_mutex_simple_lock_guard p_m(&buffer_pool_mutex);

    nas_qos_switch * p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    cps_api_return_code_t rc = cps_api_ret_code_ERR;
    nas_qos_buffer_pool *buffer_pool;
    if (buffer_pool_id) {
        buffer_pool = p_switch->get_buffer_pool(buffer_pool_id);
        if (buffer_pool == NULL)
            return NAS_QOS_E_FAIL;

        rc = _append_one_buffer_pool(param, switch_id, buffer_pool);
    }
    else {
        for (buffer_pool_iter_t it = p_switch->get_buffer_pool_it_begin();
                it != p_switch->get_buffer_pool_it_end();
                it++) {

            buffer_pool = &it->second;
            rc = _append_one_buffer_pool(param, switch_id, buffer_pool);
            if (rc != cps_api_ret_code_OK)
                return rc;
        }
    }

    return rc;
}

static cps_api_return_code_t _append_one_buffer_pool(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_buffer_pool *buffer_pool)
{
    nas_obj_id_t buffer_pool_id = buffer_pool->get_buffer_pool_id();

    /* fill in data */
    cps_api_object_t ret_obj;

    ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL){
        return cps_api_ret_code_ERR;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
            BASE_QOS_BUFFER_POOL_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_BUFFER_POOL_ID,
            cps_api_object_ATTR_T_U64,
            &buffer_pool_id, sizeof(uint64_t));
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_POOL_SHARED_SIZE,
            buffer_pool->get_shared_size());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_POOL_POOL_TYPE,
            buffer_pool->get_type());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_POOL_SIZE,
            buffer_pool->get_size());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_POOL_THRESHOLD_MODE,
            buffer_pool->get_threshold_mode());

    return cps_api_ret_code_OK;
}


/**
  * This function provides NAS-QoS buffer_pool CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_pool_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&buffer_pool_mutex);

    if (op == cps_api_oper_CREATE) {
        nas_qos_cps_api_buffer_pool_delete(obj, NULL);
    }

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_buffer_pool_set(obj, NULL);
    }

    if (op == cps_api_oper_DELETE) {
        nas_qos_cps_api_buffer_pool_create(obj, NULL);
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_buffer_pool_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{

    uint_t switch_id = 0;
    nas_obj_id_t buffer_pool_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

   nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    nas_qos_buffer_pool buffer_pool(p_switch);

    if ((rc = nas_qos_cps_parse_attr(obj, buffer_pool)) != cps_api_ret_code_OK)
        return rc;

    try {
        buffer_pool_id = p_switch->alloc_buffer_pool_id();

        buffer_pool.set_buffer_pool_id(buffer_pool_id);

        buffer_pool.commit_create(sav_obj? false: true);

        p_switch->add_buffer_pool(buffer_pool);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Created new buffer_pool %u\n",
                     buffer_pool.get_buffer_pool_id());

        // update obj with new buffer_pool-id attr and key
        cps_api_set_key_data(obj, BASE_QOS_BUFFER_POOL_ID,
                cps_api_object_ATTR_T_U64,
                &buffer_pool_id, sizeof(uint64_t));

        // save for rollback if caller requests it.
        if (sav_obj) {
            cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                 return cps_api_ret_code_ERR;
            }
            cps_api_object_clone(tmp_obj, obj);
        }

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_pool Create error code: %d ",
                    e.err_code);
        if (buffer_pool_id)
            p_switch->release_buffer_pool_id(buffer_pool_id);

        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_pool Create Unexpected error code");
        if (buffer_pool_id)
            p_switch->release_buffer_pool_id(buffer_pool_id);

        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_buffer_pool_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{

    uint_t switch_id = 0;
    nas_obj_id_t buffer_pool_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    if ((rc = nas_qos_cps_get_switch_and_buffer_pool_id(obj, switch_id, buffer_pool_id))
            !=    cps_api_ret_code_OK)
        return rc;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modify switch id %u, buffer_pool id %u\n",
                    switch_id, buffer_pool_id);

    nas_qos_buffer_pool * buffer_pool_p = nas_qos_cps_get_buffer_pool(switch_id, buffer_pool_id);
    if (buffer_pool_p == NULL) {
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing buffer_pool */
    nas_qos_buffer_pool buffer_pool(*buffer_pool_p);

    if ((rc = nas_qos_cps_parse_attr(obj, buffer_pool)) != cps_api_ret_code_OK)
        return rc;

    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying buffer_pool %u attr \n",
                     buffer_pool.get_buffer_pool_id());

        nas::attr_set_t modified_attr_list = buffer_pool.commit_modify(
                                        *buffer_pool_p, (sav_obj? false: true));

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "done with commit_modify \n");


        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current buffer_pool value
        if (sav_obj) {
            cps_api_object_t tmp_obj;
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }

            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *buffer_pool_p);

       }

        // update the local cache with newly set values
        *buffer_pool_p = buffer_pool;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_pool Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_pool Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_api_buffer_pool_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{
    uint_t switch_id = 0;
    nas_obj_id_t buffer_pool_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    if ((rc = nas_qos_cps_get_switch_and_buffer_pool_id(obj, switch_id, buffer_pool_id))
            !=    cps_api_ret_code_OK)
        return rc;


    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " switch: %u not found\n",
                     switch_id);
           return NAS_QOS_E_FAIL;
    }

    nas_qos_buffer_pool *buffer_pool_p = p_switch->get_buffer_pool(buffer_pool_id);
    if (buffer_pool_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " buffer_pool id: %u not found\n",
                     buffer_pool_id);

        return NAS_QOS_E_FAIL;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Deleting buffer_pool %u on switch: %u\n",
                 buffer_pool_p->get_buffer_pool_id(), p_switch->id());


    // delete
    try {
        buffer_pool_p->commit_delete(sav_obj? false: true);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Saving deleted buffer_pool %u\n",
                     buffer_pool_p->get_buffer_pool_id());

         // save current buffer_pool config for rollback if caller requests it.
        // use existing set_mask, existing config
        if (sav_obj) {
            cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, buffer_pool_p->set_attr_list(), *buffer_pool_p);
        }

        p_switch->remove_buffer_pool(buffer_pool_p->get_buffer_pool_id());

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_pool Delete error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;
    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_pool Delete: Unexpected error");
        return NAS_QOS_E_FAIL;
    }


    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_get_switch_and_buffer_pool_id(
                                    cps_api_object_t obj,
                                    uint_t &switch_id,
                                    nas_obj_id_t &buffer_pool_id)
{
    cps_api_object_attr_t buffer_pool_id_attr = cps_api_get_key_data(obj, BASE_QOS_BUFFER_POOL_ID);

    if (buffer_pool_id_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "buffer_pool id not exist in message");
        return NAS_QOS_E_MISSING_KEY;
    }

    switch_id = 0;
    buffer_pool_id = cps_api_object_attr_data_u64(buffer_pool_id_attr);

    return cps_api_ret_code_OK;

}

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_buffer_pool &buffer_pool)
{
    uint_t val;
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_BUFFER_POOL_ID:
            break; // These are for part of the keys

        case BASE_QOS_BUFFER_POOL_SHARED_SIZE:
            break; // READ-ONLY attribute

        case BASE_QOS_BUFFER_POOL_POOL_TYPE:
            val = cps_api_object_attr_data_u32(it.attr);
            buffer_pool.mark_attr_dirty(id);
            buffer_pool.set_type((BASE_QOS_BUFFER_POOL_TYPE_t)val);
            break;

        case BASE_QOS_BUFFER_POOL_SIZE:
            val = cps_api_object_attr_data_u32(it.attr);
            buffer_pool.mark_attr_dirty(id);
            buffer_pool.set_size(val);
            break;

        case BASE_QOS_BUFFER_POOL_THRESHOLD_MODE:
            val = cps_api_object_attr_data_u32(it.attr);
            buffer_pool.mark_attr_dirty(id);
            buffer_pool.set_threshold_mode((BASE_QOS_BUFFER_THRESHOLD_MODE_t)val);
            break;

        case CPS_API_ATTR_RESERVE_RANGE_END:
            // skip keys
            break;

        default:
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "Unrecognized option: %d", id);
            return NAS_QOS_E_UNSUPPORTED;
        }
    }


    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                    const nas::attr_set_t attr_set,
                                                    const nas_qos_buffer_pool &buffer_pool)
{
    // filling in the keys
    nas_obj_id_t buffer_pool_id = buffer_pool.get_buffer_pool_id();
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_BUFFER_POOL_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_BUFFER_POOL_ID,
            cps_api_object_ATTR_T_U64,
            &buffer_pool_id, sizeof(uint64_t));


    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_BUFFER_POOL_ID:
            /* key */
            break;

        case BASE_QOS_BUFFER_POOL_SHARED_SIZE:
            // READ-only
            break;

        case BASE_QOS_BUFFER_POOL_POOL_TYPE:
            cps_api_object_attr_add_u32(obj, attr_id, buffer_pool.get_type());
            break;

        case BASE_QOS_BUFFER_POOL_SIZE:
            cps_api_object_attr_add_u32(obj, attr_id, buffer_pool.get_size());
            break;

        case BASE_QOS_BUFFER_POOL_THRESHOLD_MODE:
            cps_api_object_attr_add_u32(obj, attr_id, buffer_pool.get_threshold_mode());
            break;

        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static nas_qos_buffer_pool * nas_qos_cps_get_buffer_pool(uint_t switch_id,
                                           nas_obj_id_t buffer_pool_id)
{

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NULL;

    nas_qos_buffer_pool *buffer_pool_p = p_switch->get_buffer_pool(buffer_pool_id);

    return buffer_pool_p;
}

static uint64_t get_stats_by_type(const nas_qos_buffer_pool_stat_counter_t *p,
                                BASE_QOS_BUFFER_POOL_STAT_t id)
{
    switch (id) {
    case BASE_QOS_BUFFER_POOL_STAT_CURRENT_OCCUPANCY_BYTES:
        return (p->current_occupancy_bytes);
    case BASE_QOS_BUFFER_POOL_STAT_WATERMARK_BYTES:
        return (p->watermark_bytes);

    default:
        return 0;
    }
}

/**
  * This function provides NAS-QoS buffer_pool stats CPS API read function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_pool_stat_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{

    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t id_attr = cps_api_get_key_data(obj, BASE_QOS_BUFFER_POOL_STAT_ID);

    if (id_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "Incomplete key: buffer pool id must be specified\n");
        return NAS_QOS_E_MISSING_KEY;
    }
    nas_obj_id_t buffer_pool_id = cps_api_object_attr_data_u64(id_attr);
    uint32_t switch_id = 0;


    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
            "Read switch id %u, buffer_pool_id %u stat\n",
            switch_id, buffer_pool_id);

    std_mutex_simple_lock_guard p_m(&buffer_pool_mutex);

    nas_qos_switch *switch_p = nas_qos_get_switch(switch_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "switch_id %u not found", switch_id);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_buffer_pool * buffer_pool_p = switch_p->get_buffer_pool(buffer_pool_id);
    if (buffer_pool_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "buffer_pool not found");
        return NAS_QOS_E_FAIL;
    }

    nas_qos_buffer_pool_stat_counter_t stats = {0};
    std::vector<BASE_QOS_BUFFER_POOL_STAT_t> counter_ids;

    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_BUFFER_POOL_STAT_ID:
            break;

        case BASE_QOS_BUFFER_POOL_STAT_CURRENT_OCCUPANCY_BYTES:
        case BASE_QOS_BUFFER_POOL_STAT_WATERMARK_BYTES:
            counter_ids.push_back((BASE_QOS_BUFFER_POOL_STAT_t)id);
            break;

        default:
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                    "Unknown buffer_pool STAT flag: %u, ignored", id);
            break;
        }
    }

    npu_id_t npu_id;
    if (buffer_pool_p->get_first_npu_id(npu_id) == false) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "npu_id not available, buffer_pool stats clear failed");
        return NAS_QOS_E_FAIL;
    }

    if (ndi_qos_get_buffer_pool_stats(npu_id,
                                buffer_pool_p->ndi_obj_id(npu_id),
                                &counter_ids[0],
                                counter_ids.size(),
                                &stats) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "buffer_pool stats get failed");
        return NAS_QOS_E_FAIL;
    }

    // return stats objects to cps-app
    cps_api_object_t ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL) {
        return cps_api_ret_code_ERR;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
            BASE_QOS_BUFFER_POOL_STAT_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_BUFFER_POOL_STAT_ID,
            cps_api_object_ATTR_T_U64,
            &buffer_pool_id, sizeof(uint64_t));

    uint64_t val64;
    for (uint_t i=0; i< counter_ids.size(); i++) {
        BASE_QOS_BUFFER_POOL_STAT_t id = counter_ids[i];
        val64 = get_stats_by_type(&stats, id);
        cps_api_object_attr_add_u64(ret_obj, id, val64);
    }

    return  cps_api_ret_code_OK;

}


