
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
 * \file   nas_qos_cps_policer.cpp
 * \brief  NAS qos policer related CPS API routines
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

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_policer &policer);
static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                const nas::attr_set_t attr_set,
                                                const nas_qos_policer &policer);
static nas_qos_policer * nas_qos_cps_get_policer(uint_t switch_id,
                                                 nas_obj_id_t policer_id);
static cps_api_return_code_t nas_qos_cps_get_switch_and_policer_id(
                                    cps_api_object_t obj,
                                    uint_t &switch_id,
                                    nas_obj_id_t &policer_id);
static cps_api_return_code_t nas_qos_cps_api_policer_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_policer_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static cps_api_return_code_t nas_qos_cps_api_policer_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj);
static bool qos_policer_get_stats_attr_is_set(cps_api_object_t obj);
static void _fill_stats(cps_api_object_t obj, cps_api_object_t ret_obj, policer_stats_struct_t &stats);
static cps_api_return_code_t _append_one_policer(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_policer * policer,
                                        cps_api_object_t obj);

static std_mutex_lock_create_static_init_rec(policer_mutex);

/**
  * This function provides NAS-QoS Policer CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_policer_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&policer_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
        return nas_qos_cps_api_policer_create(obj, param->prev);

    case cps_api_oper_SET:
        return nas_qos_cps_api_policer_set(obj, param->prev);

    case cps_api_oper_DELETE:
        return nas_qos_cps_api_policer_delete(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}


/**
  * This function provides NAS-QoS Policer CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_policer_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{

    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t policer_attr = cps_api_get_key_data(obj, BASE_QOS_METER_ID);

    uint_t switch_id = 0;
    nas_obj_id_t policer_id = (policer_attr? cps_api_object_attr_data_u64(policer_attr): 0);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, policer id %u\n",
                    switch_id, policer_id);

    std_mutex_simple_lock_guard p_m(&policer_mutex);

    nas_qos_switch * p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    cps_api_return_code_t rc = NAS_QOS_E_FAIL;
    nas_qos_policer *policer;
    if (policer_id) {
        policer = p_switch->get_policer(policer_id);
        if (policer == NULL)
            return NAS_QOS_E_FAIL;

        rc = _append_one_policer(param, switch_id, policer, obj);
    }
    else {
        for (policer_iter_t pi = p_switch->get_policer_it_begin();
                pi != p_switch->get_policer_it_end();
                pi++) {

            policer = &pi->second;
            rc = _append_one_policer(param, switch_id, policer, obj);
            if (rc != cps_api_ret_code_OK)
                return rc;
        }
    }

    return rc;
}

static cps_api_return_code_t _append_one_policer(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_policer * policer,
                                        cps_api_object_t obj)
{
    nas_obj_id_t policer_id = policer->policer_id();

    /* fill in data */
    cps_api_object_t ret_obj;

    ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL) {
        return cps_api_ret_code_ERR;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
            BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &policer_id, sizeof(uint64_t));
    cps_nas_qos_policer_struct_t cfg = policer->get_cfg();
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_METER_TYPE, cfg.ndi_cfg.meter_type);
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_METER_MODE, cfg.ndi_cfg.meter_mode);
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_METER_COLOR_SOURCE, cfg.ndi_cfg.color_source);
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_METER_GREEN_PACKET_ACTION, cfg.ndi_cfg.green_packet_action);
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_METER_YELLOW_PACKET_ACTION, cfg.ndi_cfg.yellow_packet_action);
    cps_api_object_attr_add_u32(ret_obj, BASE_QOS_METER_RED_PACKET_ACTION, cfg.ndi_cfg.red_packet_action);
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_METER_COMMITTED_BURST, cfg.ndi_cfg.committed_burst);
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_METER_COMMITTED_RATE, cfg.ndi_cfg.committed_rate);
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_METER_PEAK_BURST, cfg.ndi_cfg.peak_burst);
    cps_api_object_attr_add_u64(ret_obj, BASE_QOS_METER_PEAK_RATE, cfg.ndi_cfg.peak_rate);
    // add the list of STATS types
    for (uint_t i = 0; i< cfg.ndi_cfg.stat_list_count; i++)
        cps_api_object_attr_add_u32(ret_obj, BASE_QOS_METER_STAT_LIST, cfg.ndi_cfg.stat_list[i]);
    // add the list of NPUs
    for (auto npu_id: policer->npu_list()) {
        cps_api_object_attr_add_u32(ret_obj, BASE_QOS_METER_NPU_ID_LIST, npu_id);
    }

    // statistics needs to come from NDI directly
    policer_stats_struct_t stats = {0};
    if (qos_policer_get_stats_attr_is_set(obj)) {
        policer->poll_stats(stats);
        _fill_stats(obj, ret_obj, stats);
    }

    policer->opaque_data_to_cps (ret_obj);

    return cps_api_ret_code_OK;
}


/**
  * This function provides NAS-QoS Policer CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_policer_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&policer_mutex);

    if (op == cps_api_oper_CREATE) {
        nas_qos_cps_api_policer_delete(obj, NULL);
    }

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_policer_set(obj, NULL);
    }

    if (op == cps_api_oper_DELETE) {
        nas_qos_cps_api_policer_create(obj, NULL);
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_policer_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{

    uint_t switch_id = 0;
    nas_obj_id_t policer_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    nas_qos_policer policer(p_switch);

    if ((rc = nas_qos_cps_parse_attr(obj, policer)) != cps_api_ret_code_OK)
        return rc;

    try {
        policer_id = p_switch->alloc_policer_id();

        policer.set_policer_id(policer_id);

        policer.commit_create(sav_obj? false: true);

        p_switch->add_policer(policer);
        // policer may be changed after add_policer(), use with caution!

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Created new policer %u\n",
                     policer.policer_id());

        // update obj with new policer-id attr and key
        cps_api_set_key_data(obj, BASE_QOS_METER_ID,
                cps_api_object_ATTR_T_U64,
                &policer_id, sizeof(uint64_t));

        // update obj with opaque data upon creation
        nas_qos_policer * p = p_switch->get_policer(policer_id);
        if (p)
            p->opaque_data_to_cps (obj);

        // save for rollback if caller requests it.
        if (sav_obj) {
            cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (!tmp_obj) {
                return cps_api_ret_code_ERR;
            }
            cps_api_object_clone(tmp_obj, obj);

        }

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Policer Create error code: %d ",
                    e.err_code);
        if (policer_id)
            p_switch->release_policer_id(policer_id);

        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Policer Create Unexpected error code");
        if (policer_id)
            p_switch->release_policer_id(policer_id);

        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_policer_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{

    uint_t switch_id = 0;
    nas_obj_id_t policer_id = 0;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    if ((rc = nas_qos_cps_get_switch_and_policer_id(obj, switch_id, policer_id))
            !=    cps_api_ret_code_OK)
        return rc;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modify switch id %u, policer id %u\n",
                    switch_id, policer_id);

    nas_qos_policer * policer_p = nas_qos_cps_get_policer(switch_id, policer_id);
    if (policer_p == NULL) {
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing policer */
    nas_qos_policer policer(*policer_p);

    if ((rc = nas_qos_cps_parse_attr(obj, policer)) != cps_api_ret_code_OK)
        return rc;

    if (policer.npu_list().size() == 0) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "New Policer has no npu id on file\n");
    }
    else {
        for (auto npu_id: policer.npu_list()) {
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "New Policer npu id: %d on file\n", npu_id);
        }
    }


    if (policer_p->npu_list().size() == 0) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Existing Policer has no npu id on file\n");
    }
    else {
        for (auto npu_id: policer_p->npu_list()) {
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Existing Policer npu id: %d on file\n", npu_id);
        }
    }

    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying policer %u attr \n",
                     policer.policer_id());

        nas::attr_set_t modified_attr_list = policer.commit_modify(*policer_p, (sav_obj? false: true));

        // update obj with opaque data upon creation if it is changed
        if (policer.opaque_data_equal(policer_p) == false)
            policer.opaque_data_to_cps (obj);

        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current policer value
        if (sav_obj) {
            cps_api_object_t tmp_obj;
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (!tmp_obj) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *policer_p);
       }

        // update the local cache with newly set values
        *policer_p = policer;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Policer Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Policer Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_api_policer_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj)
{
    uint_t switch_id = 0;
    nas_obj_id_t policer_id = 0;
    if (nas_qos_cps_get_switch_and_policer_id(obj, switch_id, policer_id)
            !=    cps_api_ret_code_OK)
        return NAS_QOS_E_FAIL;

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " switch: %u not found\n",
                     switch_id);
           return NAS_QOS_E_FAIL;
    }

    nas_qos_policer *policer_p = p_switch->get_policer(policer_id);
    if (policer_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " policer id: %u not found\n",
                     policer_id);

        return NAS_QOS_E_FAIL;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Deleting policer %u on switch: %u\n",
                 policer_p->policer_id(), p_switch->id());


    // delete
    try {
        policer_p->commit_delete(sav_obj? false: true);

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Saving deleted policer %u\n",
                     policer_p->policer_id());

         // save current policer config for rollback if caller requests it.
        // use existing set_mask, existing config
        if (sav_obj) {
            cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (!tmp_obj) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, policer_p->set_attr_list(), *policer_p);
        }

        p_switch->remove_policer(policer_p->policer_id());

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Policer Delete error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;
    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Policer Delete: Unexpected error");
        return NAS_QOS_E_FAIL;
    }


    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_get_switch_and_policer_id(
                                    cps_api_object_t obj,
                                    uint_t &switch_id,
                                    nas_obj_id_t &policer_id)
{
    cps_api_object_attr_t policer_attr = cps_api_get_key_data(obj, BASE_QOS_METER_ID);

    if (policer_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", " policer_attr not exist in message");
        return NAS_QOS_E_MISSING_KEY;
    }

    switch_id = 0;
    policer_id = cps_api_object_attr_data_u64(policer_attr);

    //EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "get switch id: %u, policer_id: %u", switch_id, policer_id);

    return cps_api_ret_code_OK;

}

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
                                              nas_qos_policer &policer)
{
    npu_id_t npu_id;
    uint_t val;
    cps_nas_qos_policer_struct_t cfg = policer.get_cfg();
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_METER_SWITCH_ID:
        case BASE_QOS_METER_ID:
            break; // These are for part of the keys

        case BASE_QOS_METER_TYPE:
            val = cps_api_object_attr_data_u32(it.attr);
            if (val != BASE_QOS_METER_TYPE_PACKET &&
                val != BASE_QOS_METER_TYPE_BYTE)
                return NAS_QOS_E_ATTR_VAL;

            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.meter_type = (BASE_QOS_METER_TYPE_t)val;
            break;

        case BASE_QOS_METER_MODE:
            val = cps_api_object_attr_data_u32(it.attr);
            if (val != BASE_QOS_METER_MODE_SR_TCM &&
                val != BASE_QOS_METER_MODE_TR_TCM &&
                val != BASE_QOS_METER_MODE_STORM_CONTROL)
                return NAS_QOS_E_ATTR_VAL;

            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.meter_mode  = (BASE_QOS_METER_MODE_t)val;
            break;

        case BASE_QOS_METER_COLOR_SOURCE:
            val = cps_api_object_attr_data_u32(it.attr);
            if (val != BASE_QOS_METER_COLOR_SOURCE_BLIND &&
                val != BASE_QOS_METER_COLOR_SOURCE_AWARE)
                return NAS_QOS_E_ATTR_VAL;

            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.color_source = (BASE_QOS_METER_COLOR_SOURCE_t)val;
            break;

        case BASE_QOS_METER_GREEN_PACKET_ACTION:
            val = cps_api_object_attr_data_u32(it.attr);
            if (val != BASE_QOS_POLICER_ACTION_FORWARD &&
                val != BASE_QOS_POLICER_ACTION_DROP )
                return NAS_QOS_E_ATTR_VAL;

            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.green_packet_action = (BASE_QOS_POLICER_ACTION_t)val;
            break;

        case BASE_QOS_METER_YELLOW_PACKET_ACTION:
            val = cps_api_object_attr_data_u32(it.attr);
            if (val != BASE_QOS_POLICER_ACTION_FORWARD &&
                val != BASE_QOS_POLICER_ACTION_DROP )
                return NAS_QOS_E_ATTR_VAL;

            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.yellow_packet_action = (BASE_QOS_POLICER_ACTION_t)val;
            break;

        case BASE_QOS_METER_RED_PACKET_ACTION:
            val = cps_api_object_attr_data_u32(it.attr);
            if (val != BASE_QOS_POLICER_ACTION_FORWARD &&
                val != BASE_QOS_POLICER_ACTION_DROP )
                return NAS_QOS_E_ATTR_VAL;

            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.red_packet_action = (BASE_QOS_POLICER_ACTION_t)val;
            break;

        case BASE_QOS_METER_COMMITTED_BURST:
            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.committed_burst = cps_api_object_attr_data_u64(it.attr);
            break;

        case BASE_QOS_METER_COMMITTED_RATE:
            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.committed_rate = cps_api_object_attr_data_u64(it.attr);
            break;

        case BASE_QOS_METER_PEAK_BURST:
            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.peak_burst = cps_api_object_attr_data_u64(it.attr);
            break;

        case BASE_QOS_METER_PEAK_RATE:
            policer.mark_attr_dirty(id);
            cfg.ndi_cfg.peak_rate = cps_api_object_attr_data_u64(it.attr);
            break;

        case BASE_QOS_METER_STAT_LIST:
            if (policer.dirty_attr_list().contains(id) == false) {
                policer.mark_attr_dirty(id);

                //reset all when encountering the first STAT_LIST element
                cfg.ndi_cfg.stat_list_count = 0;
            }
            if (cfg.ndi_cfg.stat_list_count >= BASE_QOS_POLICER_STAT_TYPE_MAX) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                        "Too many statistics types specified: %d",
                        cfg.ndi_cfg.stat_list_count);
                return NAS_QOS_E_INCONSISTENT;
            }
            val = cps_api_object_attr_data_u32(it.attr);
            cfg.ndi_cfg.stat_list[cfg.ndi_cfg.stat_list_count] = (BASE_QOS_POLICER_STAT_TYPE_t)val;
            cfg.ndi_cfg.stat_list_count++;
            break;

        case BASE_QOS_METER_DATA:
            // Do not pass this flag to SAI. The flag is used between CPS and NAS.
            break;

        case BASE_QOS_METER_NPU_ID_LIST:
            npu_id = cps_api_object_attr_data_u32(it.attr);
            policer.add_npu(npu_id);
            policer.mark_attr_dirty(id);
            break;

        case CPS_API_ATTR_RESERVE_RANGE_END:
            // skip keys
            break;

        default:
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "Unrecognized option: %d", id);
            return NAS_QOS_E_UNSUPPORTED;
        }
    }

    // update the new config
    policer.set_cfg(cfg);

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                    const nas::attr_set_t attr_set,
                                                    const nas_qos_policer &policer)
{
    cps_nas_qos_policer_struct_t cfg = policer.get_cfg();

    // filling in the keys
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(obj, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &(cfg.policer_id), sizeof(uint64_t));

    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_METER_ID:
            /* key */
            break;

        case BASE_QOS_METER_TYPE:
            cps_api_object_attr_add_u32(obj, BASE_QOS_METER_TYPE,
                                        cfg.ndi_cfg.meter_type);
            break;

        case BASE_QOS_METER_MODE:
            cps_api_object_attr_add_u32(obj, BASE_QOS_METER_MODE,
                                        cfg.ndi_cfg.meter_mode);
            break;

        case BASE_QOS_METER_COLOR_SOURCE:
            cps_api_object_attr_add_u32(obj, BASE_QOS_METER_COLOR_SOURCE,
                                        cfg.ndi_cfg.color_source);
            break;

        case BASE_QOS_METER_GREEN_PACKET_ACTION:
            cps_api_object_attr_add_u32(obj, BASE_QOS_METER_GREEN_PACKET_ACTION,
                                        cfg.ndi_cfg.green_packet_action);
            break;

        case BASE_QOS_METER_YELLOW_PACKET_ACTION:
            cps_api_object_attr_add_u32(obj, BASE_QOS_METER_YELLOW_PACKET_ACTION,
                                        cfg.ndi_cfg.yellow_packet_action);
            break;

        case BASE_QOS_METER_RED_PACKET_ACTION:
            cps_api_object_attr_add_u32(obj, BASE_QOS_METER_RED_PACKET_ACTION,
                                        cfg.ndi_cfg.red_packet_action);
            break;

        case BASE_QOS_METER_COMMITTED_BURST:
            cps_api_object_attr_add_u64(obj, BASE_QOS_METER_COMMITTED_BURST,
                                        cfg.ndi_cfg.committed_burst);
            break;

        case BASE_QOS_METER_COMMITTED_RATE:
            cps_api_object_attr_add_u64(obj, BASE_QOS_METER_COMMITTED_RATE,
                                        cfg.ndi_cfg.committed_rate);
            break;

        case BASE_QOS_METER_PEAK_BURST:
            cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_BURST,
                                        cfg.ndi_cfg.peak_burst);
            break;

        case BASE_QOS_METER_PEAK_RATE:
            cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_RATE,
                                        cfg.ndi_cfg.peak_rate);
            break;

        case BASE_QOS_METER_STAT_LIST:
            for (uint_t i = 0; i< cfg.ndi_cfg.stat_list_count; i++) {
                cps_api_object_attr_add_u32(obj, BASE_QOS_METER_STAT_LIST,
                                        cfg.ndi_cfg.stat_list[i]);
            }
            break;

        case BASE_QOS_METER_DATA:
            // ndi data is not saved because rollback operation does not
            // guarantee the same ndi data anyway
            break;

        case BASE_QOS_METER_NPU_ID_LIST:
            for (auto npu_id: policer.npu_list()) {
                cps_api_object_attr_add_u32(obj, BASE_QOS_METER_NPU_ID_LIST, npu_id);
            }
            break;

        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static nas_qos_policer * nas_qos_cps_get_policer(uint_t switch_id,
                                                 nas_obj_id_t policer_id)
{

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NULL;

    nas_qos_policer *policer_p = p_switch->get_policer(policer_id);

    return policer_p;
}


static bool qos_policer_get_stats_attr_is_set(cps_api_object_t obj)
{
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);

    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_METER_PACKETS:
        case BASE_QOS_METER_BYTES:
        case BASE_QOS_METER_GREEN_PACKETS:
        case BASE_QOS_METER_GREEN_BYTES:
        case BASE_QOS_METER_YELLOW_PACKETS:
        case BASE_QOS_METER_YELLOW_BYTES:
        case BASE_QOS_METER_RED_PACKETS:
        case BASE_QOS_METER_RED_BYTES:
            return true;
        default:
            break;
        }
    }
    return false;

}

static void _fill_stats(cps_api_object_t obj, cps_api_object_t ret_obj, policer_stats_struct_t &stats)
{
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);

    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_METER_PACKETS:
            cps_api_object_attr_add_u64(ret_obj, id, stats.packets);
            break;

        case BASE_QOS_METER_BYTES:
            cps_api_object_attr_add_u64(ret_obj, id, stats.bytes);
            break;

        case BASE_QOS_METER_GREEN_PACKETS:
            cps_api_object_attr_add_u64(ret_obj, id, stats.green_packets);
            break;

        case BASE_QOS_METER_GREEN_BYTES:
            cps_api_object_attr_add_u64(ret_obj, id, stats.green_bytes);
            break;

        case BASE_QOS_METER_YELLOW_PACKETS:
            cps_api_object_attr_add_u64(ret_obj, id, stats.yellow_packets);
            break;

        case BASE_QOS_METER_YELLOW_BYTES:
            cps_api_object_attr_add_u64(ret_obj, id, stats.yellow_bytes);
            break;

        case BASE_QOS_METER_RED_PACKETS:
            cps_api_object_attr_add_u64(ret_obj, id, stats.red_packets);
            break;

        case BASE_QOS_METER_RED_BYTES:
            cps_api_object_attr_add_u64(ret_obj, id, stats.red_bytes);
            break;

        default:
            break;
        }
    }
}
