
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
 * \file   nas_qos_cps_scheduler.cpp
 * \brief  NAS qos scheduler related CPS API routines
 * \date   02-2015
 * \author
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
#include "nas_qos_scheduler.h"

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_scheduler &scheduler);
static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                const nas::attr_set_t attr_set,
                                                const nas_qos_scheduler &scheduler);
static nas_qos_scheduler * nas_qos_cps_get_scheduler(uint_t switch_id,
                                                 nas_obj_id_t scheduler_id);
static cps_api_return_code_t nas_qos_cps_api_scheduler_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_scheduler_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_scheduler_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t _append_one_scheduler(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_scheduler * scheduler);

static std_mutex_lock_create_static_init_rec(scheduler_mutex);

/**
  * This function provides NAS-QoS Scheduler CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_scheduler_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&scheduler_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
        return nas_qos_cps_api_scheduler_create(obj, param->prev);

    case cps_api_oper_SET:
        return nas_qos_cps_api_scheduler_set(obj, param->prev);

    case cps_api_oper_DELETE:
        return nas_qos_cps_api_scheduler_delete(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}


/**
  * This function provides NAS-QoS Scheduler CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_scheduler_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t scheduler_attr = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID);

    uint_t switch_id = 0;
    nas_obj_id_t scheduler_id = (scheduler_attr? cps_api_object_attr_data_u64(scheduler_attr): 0);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, scheduler id %u\n",
                    switch_id, scheduler_id);

    std_mutex_simple_lock_guard p_m(&scheduler_mutex);

    nas_qos_switch * p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    cps_api_return_code_t rc = NAS_QOS_E_FAIL;
    nas_qos_scheduler *scheduler;

    if (scheduler_id) {
        scheduler = p_switch->get_scheduler(scheduler_id);
        if (scheduler == NULL)
            return NAS_QOS_E_FAIL;

        rc = _append_one_scheduler(param, switch_id, scheduler);
    }
    else {
        for (scheduler_iter_t it = p_switch->get_scheduler_it_begin();
                it != p_switch->get_scheduler_it_end();
                it++) {

            scheduler = &it->second;
            rc = _append_one_scheduler(param, switch_id, scheduler);
            if (rc != cps_api_ret_code_OK)
                return rc;
        }
    }

    return rc;
}

static cps_api_return_code_t _append_one_scheduler(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_scheduler * scheduler)
{

    nas_obj_id_t scheduler_id = scheduler->get_scheduler_id();

    /* fill in data */
    cps_api_object_t ret_obj;

    ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL) {
        return cps_api_ret_code_ERR;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),BASE_QOS_SCHEDULER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_SCHEDULER_PROFILE_SWITCH_ID,
            cps_api_object_ATTR_T_U32,
            &switch_id, sizeof(uint32_t));
    cps_api_set_key_data(ret_obj, BASE_QOS_SCHEDULER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &scheduler_id, sizeof(uint64_t));

    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_SCHEDULER_PROFILE_ALGORITHM, scheduler->get_algorithm());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_SCHEDULER_PROFILE_WEIGHT, scheduler->get_weight());
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_SCHEDULER_PROFILE_METER_TYPE, scheduler->get_meter_type());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_SCHEDULER_PROFILE_MIN_RATE, scheduler->get_min_rate());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_SCHEDULER_PROFILE_MIN_BURST, scheduler->get_min_burst());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_SCHEDULER_PROFILE_MAX_RATE, scheduler->get_max_rate());
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_SCHEDULER_PROFILE_MAX_BURST, scheduler->get_max_burst());

    // add the list of NPUs
    for (auto npu_id: scheduler->npu_list()) {
        cps_api_object_attr_add_u32(ret_obj, BASE_QOS_SCHEDULER_PROFILE_NPU_ID_LIST, npu_id);
    }

    return cps_api_ret_code_OK;
}


/**
  * This function provides NAS-QoS Scheduler CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_scheduler_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&scheduler_mutex);

    if (op == cps_api_oper_CREATE) {
        nas_qos_cps_api_scheduler_delete(obj, NULL);
    }

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_scheduler_set(obj, NULL);
    }

    if (op == cps_api_oper_DELETE) {
        nas_qos_cps_api_scheduler_create(obj, NULL);
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_scheduler_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{
    nas_obj_id_t scheduler_id = 0;

    uint32_t switch_id = 0;

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    nas_qos_scheduler scheduler(p_switch);

    if (nas_qos_cps_parse_attr(obj, scheduler) != cps_api_ret_code_OK)
        return NAS_QOS_E_FAIL;

    try {
        scheduler_id = p_switch->alloc_scheduler_id();

        scheduler.set_scheduler_id(scheduler_id);

        scheduler.commit_create(sav_obj? false: true);

        p_switch->add_scheduler(scheduler);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Created new scheduler %u\n",
                     scheduler.get_scheduler_id());

        // update obj with new scheduler-id key
        cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID,
                cps_api_object_ATTR_T_U64,
                &scheduler_id, sizeof(uint64_t));

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
                    "NAS Scheduler Create error code: %d ",
                    e.err_code);
        if (scheduler_id)
            p_switch->release_scheduler_id(scheduler_id);

        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Scheduler Create Unexpected error code");
        if (scheduler_id)
            p_switch->release_scheduler_id(scheduler_id);

        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_scheduler_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{

    cps_api_object_attr_t scheduler_attr = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID);

    if (scheduler_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " scheduler id not specified in the message\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint_t switch_id = 0;
    nas_obj_id_t scheduler_id = cps_api_object_attr_data_u64(scheduler_attr);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modify switch id %u, scheduler id %u\n",
                    switch_id, scheduler_id);

    nas_qos_scheduler * scheduler_p = nas_qos_cps_get_scheduler(switch_id, scheduler_id);
    if (scheduler_p == NULL) {
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing scheduler */
    nas_qos_scheduler scheduler(*scheduler_p);

    if (nas_qos_cps_parse_attr(obj, scheduler) != cps_api_ret_code_OK)
        return NAS_QOS_E_FAIL;

    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying scheduler %u attr \n",
                     scheduler.get_scheduler_id());

        nas::attr_set_t modified_attr_list = scheduler.commit_modify(*scheduler_p, (sav_obj? false: true));


        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current scheduler value
        if (sav_obj) {
            cps_api_object_t tmp_obj;
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *scheduler_p);
       }

        // update the local cache with newly set values
        *scheduler_p = scheduler;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Scheduler Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Scheduler Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_api_scheduler_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{
    cps_api_object_attr_t scheduler_attr = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID);

    if (scheduler_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " scheduler id not specified in the message\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint_t switch_id = 0;
    nas_obj_id_t scheduler_id = (scheduler_attr? cps_api_object_attr_data_u64(scheduler_attr): 0);

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " switch: %u not found\n",
                     switch_id);
           return NAS_QOS_E_FAIL;
    }

    nas_qos_scheduler *scheduler_p = p_switch->get_scheduler(scheduler_id);
    if (scheduler_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " scheduler id: %u not found\n",
                     scheduler_id);

        return NAS_QOS_E_FAIL;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Deleting scheduler %u on switch: %u\n",
                 scheduler_p->get_scheduler_id(), p_switch->id());


    // delete
    try {
        scheduler_p->commit_delete(sav_obj? false: true);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Saving deleted scheduler %u\n",
                     scheduler_p->get_scheduler_id());

         // save current scheduler config for rollback if caller requests it.
        // use existing set_mask, existing config
        if (sav_obj) {
            cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, scheduler_p->set_attr_list(), *scheduler_p);
        }

        p_switch->remove_scheduler(scheduler_p->get_scheduler_id());

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Scheduler Delete error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;
    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Scheduler Delete: Unexpected error");
        return NAS_QOS_E_FAIL;
    }


    return cps_api_ret_code_OK;
}


/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_scheduler &scheduler)
{
    npu_id_t npu_id;
    uint_t val;
    uint64_t val64;
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_SCHEDULER_PROFILE_SWITCH_ID:
        case BASE_QOS_SCHEDULER_PROFILE_ID:
            break; // These are for part of the keys

        case BASE_QOS_SCHEDULER_PROFILE_ALGORITHM:
            val = cps_api_object_attr_data_u32(it.attr);
            scheduler.set_algorithm((BASE_QOS_SCHEDULING_TYPE_t)val);
            break;

        case BASE_QOS_SCHEDULER_PROFILE_WEIGHT:
            val = cps_api_object_attr_data_u32(it.attr);
            scheduler.set_weight(val);
            break;

        case BASE_QOS_SCHEDULER_PROFILE_METER_TYPE:
            val = cps_api_object_attr_data_u32(it.attr);
            scheduler.set_meter_type((BASE_QOS_METER_TYPE_t)val);
            break;

        case BASE_QOS_SCHEDULER_PROFILE_MIN_RATE:
            val64 = cps_api_object_attr_data_u64(it.attr);
            scheduler.set_min_rate(val64);
            break;

        case BASE_QOS_SCHEDULER_PROFILE_MIN_BURST:
            val64 = cps_api_object_attr_data_u64(it.attr);
            scheduler.set_min_burst(val64);
            break;

        case BASE_QOS_SCHEDULER_PROFILE_MAX_RATE:
            val64 = cps_api_object_attr_data_u64(it.attr);
            scheduler.set_max_rate(val64);
            break;

        case BASE_QOS_SCHEDULER_PROFILE_MAX_BURST:
            val64 = cps_api_object_attr_data_u64(it.attr);
            scheduler.set_max_burst(val64);
            break;

        case BASE_QOS_SCHEDULER_PROFILE_NPU_ID_LIST:
            npu_id = cps_api_object_attr_data_u32(it.attr);
            scheduler.add_npu(npu_id);
            scheduler.mark_attr_dirty(id);
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
                                                    const nas_qos_scheduler &scheduler)
{
    // filling in the keys
    uint32_t switch_id = scheduler.switch_id();
    nas_obj_id_t scheduler_id = scheduler.get_scheduler_id();
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_SWITCH_ID,
            cps_api_object_ATTR_T_U32,
            &switch_id, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &scheduler_id, sizeof(uint64_t));



    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_SCHEDULER_PROFILE_ID:
            /* key */
            break;

        case BASE_QOS_SCHEDULER_PROFILE_ALGORITHM:
            cps_api_object_attr_add_u32(obj, attr_id, scheduler.get_algorithm());
            break;

        case BASE_QOS_SCHEDULER_PROFILE_WEIGHT:
            cps_api_object_attr_add_u32(obj, attr_id, scheduler.get_weight());
            break;

        case BASE_QOS_SCHEDULER_PROFILE_METER_TYPE:
            cps_api_object_attr_add_u32(obj, attr_id, scheduler.get_meter_type());
            break;

        case BASE_QOS_SCHEDULER_PROFILE_MIN_RATE:
            cps_api_object_attr_add_u64(obj, attr_id, scheduler.get_min_rate());
            break;

        case BASE_QOS_SCHEDULER_PROFILE_MIN_BURST:
            cps_api_object_attr_add_u64(obj, attr_id, scheduler.get_min_burst());
            break;

        case BASE_QOS_SCHEDULER_PROFILE_MAX_RATE:
            cps_api_object_attr_add_u64(obj, attr_id, scheduler.get_max_rate());
            break;

        case BASE_QOS_SCHEDULER_PROFILE_MAX_BURST:
            cps_api_object_attr_add_u64(obj, attr_id, scheduler.get_max_burst());
            break;

        case BASE_QOS_SCHEDULER_PROFILE_NPU_ID_LIST:
            for (auto npu_id: scheduler.npu_list()) {
                cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_NPU_ID_LIST, npu_id);
            }
            break;
        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static nas_qos_scheduler * nas_qos_cps_get_scheduler(uint_t switch_id,
                                                 nas_obj_id_t scheduler_id)
{

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NULL;

    nas_qos_scheduler *scheduler_p = p_switch->get_scheduler(scheduler_id);

    return scheduler_p;
}


