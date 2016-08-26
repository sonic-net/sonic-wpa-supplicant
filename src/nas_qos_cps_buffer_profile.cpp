
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
 * \file   nas_qos_cps_buffer_profile.cpp
 * \brief  NAS qos buffer_profile related CPS API routines
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
#include "nas_qos_buffer_profile.h"

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_buffer_profile &buffer_profile);
static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                const nas::attr_set_t attr_set,
                                                const nas_qos_buffer_profile &buffer_profile);
static nas_qos_buffer_profile * nas_qos_cps_get_buffer_profile(uint_t switch_id,
                                           nas_obj_id_t buffer_profile_id);
static cps_api_return_code_t nas_qos_cps_get_switch_and_buffer_profile_id(
                                    cps_api_object_t obj,
                                    uint_t &switch_id,
                                    nas_obj_id_t &buffer_profile_id);
static cps_api_return_code_t nas_qos_cps_api_buffer_profile_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_buffer_profile_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_buffer_profile_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t _append_one_buffer_profile(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_buffer_profile *buffer_profile);

static std_mutex_lock_create_static_init_rec(buffer_profile_mutex);

/**
  * This function provides NAS-QoS buffer_profile CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_profile_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&buffer_profile_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
        return nas_qos_cps_api_buffer_profile_create(obj, param->prev);

    case cps_api_oper_SET:
        return nas_qos_cps_api_buffer_profile_set(obj, param->prev);

    case cps_api_oper_DELETE:
        return nas_qos_cps_api_buffer_profile_delete(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}


/**
  * This function provides NAS-QoS buffer_profile CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_profile_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t buffer_profile_id_attr = cps_api_get_key_data(obj, BASE_QOS_BUFFER_PROFILE_ID);

    uint_t switch_id = 0;
    nas_obj_id_t buffer_profile_id = (buffer_profile_id_attr?
                    cps_api_object_attr_data_u64(buffer_profile_id_attr): 0);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, buffer_profile id %u\n",
                    switch_id, buffer_profile_id);

    std_mutex_simple_lock_guard p_m(&buffer_profile_mutex);

    nas_qos_switch * p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    cps_api_return_code_t rc = cps_api_ret_code_ERR;
    nas_qos_buffer_profile *buffer_profile;
    if (buffer_profile_id) {
        buffer_profile = p_switch->get_buffer_profile(buffer_profile_id);
        if (buffer_profile == NULL)
            return NAS_QOS_E_FAIL;

        rc = _append_one_buffer_profile(param, switch_id, buffer_profile);
    }
    else {
        for (buffer_profile_iter_t it = p_switch->get_buffer_profile_it_begin();
                it != p_switch->get_buffer_profile_it_end();
                it++) {

            buffer_profile = &it->second;
            rc = _append_one_buffer_profile(param, switch_id, buffer_profile);
            if (rc != cps_api_ret_code_OK)
                return rc;
        }
    }

    return rc;
}

static cps_api_return_code_t _append_one_buffer_profile(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_buffer_profile *buffer_profile)
{
    nas_obj_id_t buffer_profile_id = buffer_profile->get_buffer_profile_id();

    /* fill in data */
    cps_api_object_t ret_obj;

    ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL){
        return NAS_QOS_E_FAIL;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
            BASE_QOS_BUFFER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_BUFFER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &buffer_profile_id, sizeof(uint64_t));
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_BUFFER_PROFILE_POOL_ID,
            buffer_profile->get_buffer_pool_id());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_PROFILE_BUFFER_SIZE,
            buffer_profile->get_buffer_size());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_PROFILE_SHARED_DYNAMIC_THRESHOLD,
            buffer_profile->get_shared_dynamic_th());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_PROFILE_SHARED_STATIC_THRESHOLD,
            buffer_profile->get_shared_static_th());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_PROFILE_XOFF_THRESHOLD,
            buffer_profile->get_xoff_th());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_BUFFER_PROFILE_XON_THRESHOLD,
            buffer_profile->get_xon_th());

    return cps_api_ret_code_OK;
}


/**
  * This function provides NAS-QoS buffer_profile CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_profile_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&buffer_profile_mutex);

    if (op == cps_api_oper_CREATE) {
        nas_qos_cps_api_buffer_profile_delete(obj, NULL);
    }

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_buffer_profile_set(obj, NULL);
    }

    if (op == cps_api_oper_DELETE) {
        nas_qos_cps_api_buffer_profile_create(obj, NULL);
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_buffer_profile_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{

    uint_t switch_id = 0;
    nas_obj_id_t buffer_profile_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    nas_qos_buffer_profile buffer_profile(p_switch);

    if ((rc = nas_qos_cps_parse_attr(obj, buffer_profile)) != cps_api_ret_code_OK)
        return rc;

    try {
        buffer_profile_id = p_switch->alloc_buffer_profile_id();

        buffer_profile.set_buffer_profile_id(buffer_profile_id);

        buffer_profile.commit_create(sav_obj? false: true);

        p_switch->add_buffer_profile(buffer_profile);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Created new buffer_profile %u\n",
                     buffer_profile.get_buffer_profile_id());

        // update obj with new buffer_profile-id attr and key
        cps_api_set_key_data(obj, BASE_QOS_BUFFER_PROFILE_ID,
                cps_api_object_ATTR_T_U64,
                &buffer_profile_id, sizeof(uint64_t));

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
                    "NAS buffer_profile Create error code: %d ",
                    e.err_code);
        if (buffer_profile_id)
            p_switch->release_buffer_profile_id(buffer_profile_id);

        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_profile Create Unexpected error code");
        if (buffer_profile_id)
            p_switch->release_buffer_profile_id(buffer_profile_id);

        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_buffer_profile_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{

    uint_t switch_id = 0;
    nas_obj_id_t buffer_profile_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    if ((rc = nas_qos_cps_get_switch_and_buffer_profile_id(obj, switch_id, buffer_profile_id))
            !=    cps_api_ret_code_OK)
        return rc;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modify switch id %u, buffer_profile id %u\n",
                    switch_id, buffer_profile_id);

    nas_qos_buffer_profile * buffer_profile_p = nas_qos_cps_get_buffer_profile(switch_id, buffer_profile_id);
    if (buffer_profile_p == NULL) {
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing buffer_profile */
    nas_qos_buffer_profile buffer_profile(*buffer_profile_p);

    if ((rc = nas_qos_cps_parse_attr(obj, buffer_profile)) != cps_api_ret_code_OK)
        return rc;

    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying buffer_profile %u attr \n",
                     buffer_profile.get_buffer_profile_id());

        nas::attr_set_t modified_attr_list = buffer_profile.commit_modify(
                                    *buffer_profile_p, (sav_obj? false: true));

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "done with commit_modify \n");


        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current buffer_profile value
        if (sav_obj) {
            cps_api_object_t tmp_obj;
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }

            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *buffer_profile_p);

       }

        // update the local cache with newly set values
        *buffer_profile_p = buffer_profile;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_profile Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_profile Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_api_buffer_profile_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{
    uint_t switch_id = 0;
    nas_obj_id_t buffer_profile_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    if ((rc = nas_qos_cps_get_switch_and_buffer_profile_id(obj, switch_id, buffer_profile_id))
            !=    cps_api_ret_code_OK)
        return rc;


    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " switch: %u not found\n",
                     switch_id);
           return NAS_QOS_E_FAIL;
    }

    nas_qos_buffer_profile *buffer_profile_p = p_switch->get_buffer_profile(buffer_profile_id);
    if (buffer_profile_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " buffer_profile id: %u not found\n",
                     buffer_profile_id);

        return NAS_QOS_E_FAIL;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Deleting buffer_profile %u on switch: %u\n",
                 buffer_profile_p->get_buffer_profile_id(), p_switch->id());


    // delete
    try {
        buffer_profile_p->commit_delete(sav_obj? false: true);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Saving deleted buffer_profile %u\n",
                     buffer_profile_p->get_buffer_profile_id());

         // save current buffer_profile config for rollback if caller requests it.
        // use existing set_mask, existing config
        if (sav_obj) {
            cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, buffer_profile_p->set_attr_list(), *buffer_profile_p);
        }

        p_switch->remove_buffer_profile(buffer_profile_p->get_buffer_profile_id());

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_profile Delete error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;
    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS buffer_profile Delete: Unexpected error");
        return NAS_QOS_E_FAIL;
    }


    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_get_switch_and_buffer_profile_id(
                                    cps_api_object_t obj,
                                    uint_t &switch_id,
                                    nas_obj_id_t &buffer_profile_id)
{
    cps_api_object_attr_t buffer_profile_id_attr = cps_api_get_key_data(obj, BASE_QOS_BUFFER_PROFILE_ID);

    if (buffer_profile_id_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "buffer_profile id not exist in message");
        return NAS_QOS_E_MISSING_KEY;
    }

    switch_id = 0;
    buffer_profile_id = cps_api_object_attr_data_u64(buffer_profile_id_attr);

    return cps_api_ret_code_OK;

}

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_buffer_profile &buffer_profile)
{
    uint_t val;
    nas_obj_id_t pool_id;
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_BUFFER_PROFILE_ID:
            break; // These are for part of the keys

        case BASE_QOS_BUFFER_PROFILE_POOL_ID:
            pool_id = cps_api_object_attr_data_u64(it.attr);
            buffer_profile.mark_attr_dirty(id);
            buffer_profile.set_buffer_pool_id(pool_id);
            break;


        case BASE_QOS_BUFFER_PROFILE_BUFFER_SIZE:
            val = cps_api_object_attr_data_u32(it.attr);
            buffer_profile.mark_attr_dirty(id);
            buffer_profile.set_buffer_size(val);
            break;

        case BASE_QOS_BUFFER_PROFILE_SHARED_DYNAMIC_THRESHOLD:
            val = cps_api_object_attr_data_u32(it.attr);
            buffer_profile.mark_attr_dirty(id);
            buffer_profile.set_shared_dynamic_th(val);
            break;

        case BASE_QOS_BUFFER_PROFILE_SHARED_STATIC_THRESHOLD:
            val = cps_api_object_attr_data_u32(it.attr);
            buffer_profile.mark_attr_dirty(id);
            buffer_profile.set_shared_static_th(val);
            break;

        case BASE_QOS_BUFFER_PROFILE_XOFF_THRESHOLD:
            val = cps_api_object_attr_data_u32(it.attr);
            buffer_profile.mark_attr_dirty(id);
            buffer_profile.set_xoff_th(val);
            break;

        case BASE_QOS_BUFFER_PROFILE_XON_THRESHOLD:
            val = cps_api_object_attr_data_u32(it.attr);
            buffer_profile.mark_attr_dirty(id);
            buffer_profile.set_xon_th(val);
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
                                                    const nas_qos_buffer_profile &buffer_profile)
{
    // filling in the keys
    nas_obj_id_t buffer_profile_id = buffer_profile.get_buffer_profile_id();
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_BUFFER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_BUFFER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &buffer_profile_id, sizeof(uint64_t));


    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_BUFFER_PROFILE_ID:
            /* key */
            break;

        case BASE_QOS_BUFFER_PROFILE_POOL_ID:
            cps_api_object_attr_add_u64(obj, attr_id,
                    buffer_profile.get_buffer_pool_id());
            break;

        case BASE_QOS_BUFFER_PROFILE_BUFFER_SIZE:
            cps_api_object_attr_add_u32(obj, attr_id,
                    buffer_profile.get_buffer_size());
            break;

        case BASE_QOS_BUFFER_PROFILE_SHARED_DYNAMIC_THRESHOLD:
            cps_api_object_attr_add_u32(obj, attr_id,
                    buffer_profile.get_shared_dynamic_th());
            break;

        case BASE_QOS_BUFFER_PROFILE_SHARED_STATIC_THRESHOLD:
            cps_api_object_attr_add_u32(obj, attr_id,
                    buffer_profile.get_shared_static_th());
            break;

        case BASE_QOS_BUFFER_PROFILE_XOFF_THRESHOLD:
            cps_api_object_attr_add_u32(obj, attr_id,
                    buffer_profile.get_xoff_th());
            break;

        case BASE_QOS_BUFFER_PROFILE_XON_THRESHOLD:
            cps_api_object_attr_add_u32(obj, attr_id,
                    buffer_profile.get_xon_th());
            break;

        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static nas_qos_buffer_profile * nas_qos_cps_get_buffer_profile(uint_t switch_id,
                                           nas_obj_id_t buffer_profile_id)
{

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NULL;

    nas_qos_buffer_profile *buffer_profile_p = p_switch->get_buffer_profile(buffer_profile_id);

    return buffer_profile_p;
}


