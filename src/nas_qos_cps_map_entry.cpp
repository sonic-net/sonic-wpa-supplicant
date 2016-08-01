
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
 * \file   nas_qos_cps_map_entry.cpp
 * \brief  CPS API routines for dot1p|dscp to tc/color maps
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

#include "nas_qos_common.h"
#include "nas_qos_switch_list.h"
#include "nas_qos_cps.h"
#include "cps_api_key.h"
#include "dell-base-qos.h"
#include "nas_qos_map.h"
#include "nas_qos_map_util.h"

#include <unordered_map>
#include <set>

/* Parse the attributes */
static cps_api_return_code_t nas_qos_cps_parse_attr(cps_api_object_t obj,
                                nas_qos_map_entry  &entry );
static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                const nas::attr_set_t attr_set,
                                                const nas_qos_map &map);
static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                const nas::attr_set_t attr_set,
                                                const nas_qos_map_entry &entry);
static cps_api_return_code_t nas_qos_cps_api_map_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj,
                                nas_qos_map_type_t map_type);
static cps_api_return_code_t nas_qos_cps_api_map_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj,
                                nas_qos_map_type_t map_type);
static cps_api_return_code_t nas_qos_cps_api_map_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj,
                                nas_qos_map_type_t map_type);
static bool qos_map_entry_attr_is_set(cps_api_object_t obj,
                                nas_qos_map_type_t map_type);
static cps_api_return_code_t _fill_matching_map_entries(
                                cps_api_object_t ret_obj,
                                nas_qos_map_entry_key_t search_key,
                                map_entries *entries,
                                nas_qos_map_type_t map_type);
static cps_api_return_code_t _fill_matching_map(
                                    cps_api_object_t ret_obj,
                                    map_entries *entries,
                                    nas_qos_map_type_t map_type);
static cps_api_return_code_t nas_qos_cps_validate_set_attr_list(
                                nas_qos_map_entry & entry,
                                nas_qos_map_type_t map_type);
static void _fill_entry_key_data(cps_api_object_t ret_obj,
        nas_qos_map_entry_key_t key,
        nas_qos_map_type_t map_type);

static bool qos_map_e_add_int_by_type (cps_api_object_t obj,
        cps_api_attr_id_t *id,  size_t id_size,
        uint val, cps_api_object_ATTR_TYPE_t type);

static bool qos_map_add_int_by_type(cps_api_object_t obj,
        cps_api_attr_id_t id,
        uint_t val,
        cps_api_object_ATTR_TYPE_t type);
static cps_api_return_code_t _append_one_map(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_map *map,
                                        nas_qos_map_type_t map_type,
                                        nas_qos_map_entry_key_t search_key,
                                        bool is_map_entry
                                        );


/**
  * This function provides NAS-QoS Map CPS API write function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_map_write_type(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix,
                                            nas_qos_map_type_t map_type)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    switch (op) {
    case cps_api_oper_CREATE:
        return nas_qos_cps_api_map_create(obj, param->prev, map_type);

    case cps_api_oper_SET:
        return nas_qos_cps_api_map_set(obj, param->prev, map_type);

    case cps_api_oper_DELETE:
        return nas_qos_cps_api_map_delete(obj, param->prev, map_type);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}

/**
  * This function provides NAS-QoS Map CPS API read function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_map_read_type (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix,
                                            nas_qos_map_type_t map_type)
{
    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);

    nas_attr_id_t obj_id;

    uint_t switch_id = 0;

    obj_id = qos_map_map_id_obj_id(map_type);
    cps_api_object_attr_t map_attr = cps_api_get_key_data(obj, obj_id);
    nas_obj_id_t map_id = (map_attr? cps_api_object_attr_data_u64(map_attr): 0);

    // check the request is on Map level or Entry level
    bool is_map_entry;
    cps_api_return_code_t rc = cps_api_ret_code_OK;
    if ((rc = qos_map_key_check(obj, map_type, &is_map_entry)) != cps_api_ret_code_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Key error");
        return rc;
    }

    obj_id = qos_map_entry_key_1_obj_id(map_type);
    cps_api_object_attr_t entry_attr = cps_api_get_key_data(obj, obj_id);
    if (entry_attr == NULL && is_map_entry) {
        // Inner list does not support wildcard get.
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Key value must be specified for inner list\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    nas_qos_map_entry_key_t search_key;
    if (entry_attr) {
        search_key.any = *(uint8_t *)cps_api_object_attr_data_bin(entry_attr);
    }

    if (map_type == NDI_QOS_MAP_TC_TO_QUEUE ||
        map_type == NDI_QOS_MAP_TC_COLOR_TO_DOT1P ||
        map_type == NDI_QOS_MAP_TC_COLOR_TO_DSCP ||
        map_type == NDI_QOS_MAP_PG_TO_PFC ||
        map_type == NDI_QOS_MAP_PFC_TO_QUEUE) {
        obj_id = qos_map_entry_key_2_obj_id(map_type);
        cps_api_object_attr_t entry_key2_attr = cps_api_get_key_data(obj, obj_id);
        if (entry_key2_attr == NULL && is_map_entry) {
            // Inner list does not support wildcard get.
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                        "Secondary Key value missing for inner list\n");
            return NAS_QOS_E_MISSING_KEY;
        }

        if (entry_key2_attr) {
            uint32_t key2;

            if (map_type == NDI_QOS_MAP_PG_TO_PFC)
                key2 = *(uint8_t *)cps_api_object_attr_data_bin(entry_key2_attr);
            else
                key2 = cps_api_object_attr_data_u32(entry_key2_attr);

            // form a complex key
            uint_t comp_key = MAKE_KEY(search_key.any, key2);

            if (map_type == NDI_QOS_MAP_TC_TO_QUEUE)
                search_key.tc_queue_type = comp_key;
            else if (map_type == NDI_QOS_MAP_TC_COLOR_TO_DOT1P ||
                     map_type == NDI_QOS_MAP_TC_COLOR_TO_DSCP)
                search_key.tc_color = comp_key;
            else if (map_type == NDI_QOS_MAP_PG_TO_PFC)
                search_key.pg_prio = comp_key;
            else if (map_type == NDI_QOS_MAP_PFC_TO_QUEUE)
                search_key.prio_queue_type = comp_key;
        }
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, map id %u\n",
                    switch_id, map_id);

    nas_qos_switch * p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    nas_qos_map * map;
    if (map_id) {
        map = p_switch->get_map(map_id);
        if (map == NULL)
            return NAS_QOS_E_FAIL;

        rc = _append_one_map(param, switch_id, map, map_type, search_key, is_map_entry);
    }
    else {
        // map id is not specified
        for (map_iter_t it = p_switch->get_map_it_begin();
                it != p_switch->get_map_it_end();
                it++) {

            map = &it->second;
            rc = _append_one_map(param, switch_id, map, map_type, search_key, false);
            if (rc != cps_api_ret_code_OK)
                return rc;
        }
    }

    return rc;
}

static cps_api_return_code_t _append_one_map(cps_api_get_params_t * param,
                                        uint_t switch_id,
                                        nas_qos_map *map,
                                        nas_qos_map_type_t map_type,
                                        nas_qos_map_entry_key_t search_key,
                                        bool is_map_entry
                                        )
{
    /* fill in data */
    cps_api_object_t ret_obj;

    ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL) {
        return cps_api_ret_code_ERR;
    }

    nas_attr_id_t obj_id = (is_map_entry? qos_map_entry_obj_id(map_type):
                            qos_map_obj_id(map_type));
    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
            obj_id,
            cps_api_qualifier_TARGET);

    obj_id = qos_map_map_id_obj_id(map_type);
    nas_obj_id_t map_id = map->get_map_id();
    cps_api_set_key_data(ret_obj, obj_id,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    map_entries *entries = map->get_map_entries();
    if (entries == NULL)
        return NAS_QOS_E_FAIL;

    if (is_map_entry) {
        return _fill_matching_map_entries(ret_obj, search_key, entries, map_type);
    }
    else {
        return _fill_matching_map(ret_obj, entries, map_type);
    }
}


/**
  * This function provides NAS-QoS Map CPS API rollback function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_map_rollback_type(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix,
                                            nas_qos_map_type_t map_type)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    if (op == cps_api_oper_CREATE) {
        nas_qos_cps_api_map_delete(obj, NULL, map_type);
    }

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_map_set(obj, NULL, map_type);
    }

    if (op == cps_api_oper_DELETE) {
        nas_qos_cps_api_map_create(obj, NULL, map_type);
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_map_create(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj,
                                nas_qos_map_type_t map_type)
{
    uint_t switch_id;
    nas_obj_id_t map_id = 0;
    nas_qos_map_entry_key_t key;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    // check the request is on Map level or Entry level
    bool is_map_entry;
    if ((rc = qos_map_key_check(obj, map_type, &is_map_entry)) != cps_api_ret_code_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Key error");
        return rc;
    }

    if (is_map_entry) {
        if ((rc = qos_map_entry_get_key(obj, map_type, switch_id, map_id, key)) != cps_api_ret_code_OK)
            return rc;

    }
    else {
        if ((rc = qos_map_get_switch_id(obj, map_type, switch_id)) != cps_api_ret_code_OK)
            return rc;

        // while creating map id, do not accept map-entry attributes.
        // Map-entry needs to be created separately.
        if (qos_map_entry_attr_is_set(obj, map_type)) {
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "Inner list map entries must be created separately after map id is created\n");
            return NAS_QOS_E_INCONSISTENT;
        }
    }

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    try {
        if (!is_map_entry) {
            // Map id create

            nas_qos_map map(p_switch, map_type);

            map_id = p_switch->alloc_map_id();

            map.set_map_id(map_id);

            map.commit_create(sav_obj? false: true);

            p_switch->add_map(map);

            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Created new map %u\n",
                         map.get_map_id());

            // update obj with new map-id attr and key
            nas_attr_id_t map_id_obj_id = qos_map_map_id_obj_id(map_type);
            cps_api_set_key_data(obj,
                    map_id_obj_id,
                    cps_api_object_ATTR_T_U64,
                    &map_id, sizeof(uint64_t));

            // save for rollback if caller requests it.
            if (sav_obj) {
                cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
                if (tmp_obj == NULL) {
                    return cps_api_ret_code_ERR;
                }
                cps_api_object_clone(tmp_obj, obj);
            }
        }
        else {
            // Map entry create
            nas_qos_map * p_map = p_switch->get_map(map_id);
            if (p_map == NULL) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                        "NAS map id %lu is not found", map_id);
                return NAS_QOS_E_FAIL;
            }

            if (p_map->get_type() != map_type) {
                EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                        "NAS map id %lu has mismatched type: %d vs expected %d",
                        map_id, p_map->get_type(), map_type);
                return NAS_QOS_E_INCONSISTENT;
            }

            nas_qos_map_entry entry(p_switch, map_id, map_type, key);

            if ((rc = nas_qos_cps_parse_attr(obj, entry)) != cps_api_ret_code_OK)
                return rc;

            //push a create to NPUs
            entry.commit_create(sav_obj? false: true);

            p_map->add_map_entry(key, entry);

            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                    "Created new map entry with key %u\n",
                         key.any);

            // save for rollback if caller requests it.
            if (sav_obj) {
                cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
                if (tmp_obj == NULL) {
                    return cps_api_ret_code_ERR;
                }
                cps_api_object_clone(tmp_obj, obj);
            }
        }
    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS %s Create error code: %d ",
                    (is_map_entry? "Map Entry": "Map"),
                    e.err_code);
        if (!is_map_entry && map_id)
            p_switch->release_map_id(map_id);

        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS %s Create Unexpected error code",
                    (is_map_entry? "Map Entry": "Map"));
        if (!is_map_entry && map_id)
            p_switch->release_map_id(map_id);

        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_api_map_set(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj,
                                nas_qos_map_type_t map_type)
{
    uint_t switch_id;
    nas_obj_id_t map_id;
    nas_qos_map_entry_key_t key;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    if ((rc = qos_map_entry_get_key(obj, map_type, switch_id, map_id, key)) != cps_api_ret_code_OK)
        return rc;

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modify switch id %u, map id %u, key %u\n",
                    switch_id, map_id, key.any);

    nas_qos_map * map_p = nas_qos_cps_get_map(switch_id, map_id);
    if (map_p == NULL) {
        return NAS_QOS_E_FAIL;
    }

    map_entries  * entries = map_p->get_map_entries();
    if (entries == NULL) {
        return NAS_QOS_E_FAIL;
    }
    map_entries_iter_t it = entries->find(key.any);
    if (it == entries->end()) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Map entry does not exist in NAS\n");
        return NAS_QOS_E_KEY_VAL;
    }
    nas_qos_map_entry & entry = entries->at(key.any);

    /* make a local copy of the existing map entry*/
    nas_qos_map_entry old_entry(entry);

    if ((rc = nas_qos_cps_parse_attr(obj, entry)) != cps_api_ret_code_OK)
        return rc;

    if ((rc = nas_qos_cps_validate_set_attr_list(entry, map_type)) != cps_api_ret_code_OK)
        return rc;

    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying map %u attr \n",
                     entry.get_map_id());

        if (entry.is_created_in_ndi() == false)
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "map is yet not created in ndi \n");

        nas::attr_set_t modified_attr_list = entry.commit_modify(old_entry, (sav_obj? false: true));

        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current map value
        if (sav_obj) {
            cps_api_object_t tmp_obj;
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (tmp_obj == NULL) {
                return cps_api_ret_code_ERR;
            }

            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, old_entry);
        }

        // update the local cache with newly set values
        old_entry = entry;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Map Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Map Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_api_map_delete(
                                cps_api_object_t obj,
                                cps_api_object_list_t sav_obj,
                                nas_qos_map_type_t map_type)
{
    uint_t switch_id;
    nas_obj_id_t map_id;
    nas_qos_map_entry_key_t key;
    cps_api_return_code_t rc = cps_api_ret_code_OK;

    // check the request is on Map level or Entry level
    bool is_map_entry;
    if ((rc = qos_map_key_check(obj, map_type, &is_map_entry)) != cps_api_ret_code_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Key error");
        return rc;
    }

    if (is_map_entry) {
        if ((rc = qos_map_entry_get_key(obj, map_type, switch_id, map_id, key)) != cps_api_ret_code_OK)
            return rc;
    }
    else {
        if ((rc = qos_map_get_key(obj, map_type, switch_id, map_id)) != cps_api_ret_code_OK)
            return rc;
    }

    nas_qos_map * map_p = nas_qos_cps_get_map(switch_id, map_id);
    if (map_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", " map id: %u not found\n",
                     map_id);

        return NAS_QOS_E_FAIL;
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Deleting map %u on switch: %u\n",
                 map_id, switch_id);

    if (!is_map_entry) {
        // deleting a map requires deleting all map-entries first
        map_entries  * entries = map_p->get_map_entries();

        if (entries && entries->size() != 0) {
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                    "Map entry exists (%d), cannot delete the map\n",
                    entries->size());

            return NAS_QOS_E_INCONSISTENT;
        }
    }

    // delete
    try {
        if (!is_map_entry) {
            //Map level

            map_p->commit_delete(sav_obj? false: true);

            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Saving deleted map %u\n",
                         map_p->get_map_id());

            // save current map config for rollback if caller requests it.
            // use existing set_mask, existing config
            if (sav_obj) {
                cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
                if (tmp_obj == NULL) {
                    return cps_api_ret_code_ERR;
                }
                nas_qos_store_prev_attr(tmp_obj, map_p->set_attr_list(), *map_p);
            }

            const_cast<nas_qos_switch &>(map_p->get_switch()).remove_map(map_id);
        }
        else {
            // map entry level
            map_entries  * entries = map_p->get_map_entries();
            if (entries == NULL)
                return NAS_QOS_E_FAIL;

            nas_qos_map_entry entry = entries->at(key.any);

            entry.commit_delete(sav_obj? false: true);

            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Saving deleted map %u entry, key %u\n",
                         map_p->get_map_id(), key.any);

            // save current map config for rollback if caller requests it.
            // use existing set_mask, existing config
            if (sav_obj) {
                cps_api_object_t tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
                if (tmp_obj == NULL) {
                    return NAS_QOS_E_FAIL;
                }
                nas_qos_store_prev_attr(tmp_obj, entry.set_attr_list(), entry);
            }

            map_p->del_map_entry(key);

        }
    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Map Delete error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;
    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS Map Delete: Unexpected error");
        return NAS_QOS_E_FAIL;
    }


    return cps_api_ret_code_OK;
}


/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_attr(cps_api_object_t obj,
        nas_qos_map_entry  &entry )
{
    nas_qos_map_type_t type = entry.get_type();

    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        _attr_parse_fn fn;

        if (id == qos_map_entry_attr_1_obj_id(type)) {
            fn = qos_map_entry_attr_1_parse_fn(type);
            fn(it.attr, entry);
        }

        if (id == qos_map_entry_attr_2_obj_id(type)) {
            fn = qos_map_entry_attr_2_parse_fn(type);
            fn(it.attr, entry);
        }
    }

    return cps_api_ret_code_OK;
}


static bool qos_map_entry_attr_is_set(cps_api_object_t obj, nas_qos_map_type_t type)
{
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);

    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        if (id == qos_map_entry_obj_id(type) ||
            id == qos_map_entry_key_1_obj_id(type) ||
            id == qos_map_entry_key_2_obj_id(type) ||
            id == qos_map_entry_attr_1_obj_id(type) ||
            id == qos_map_entry_attr_2_obj_id(type))
            return true; //found
    }

    return false;

}

static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                    const nas::attr_set_t attr_set,
                                                    const nas_qos_map &map)
{
    // fill in keys
    nas_obj_id_t map_id = map.get_map_id();
    nas_qos_map_type_t map_type = map.get_type();

    nas_attr_id_t obj_id = qos_map_obj_id(map_type);
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
            obj_id,
            cps_api_qualifier_TARGET);

    obj_id = qos_map_map_id_obj_id(map_type);
    cps_api_set_key_data(obj,
            obj_id,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    // Attributes: no-op

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                    const nas::attr_set_t attr_set,
                                                    const nas_qos_map_entry &entry)
{
    // fill in keys
    nas_qos_map_entry_key_t key = entry.get_key();
    nas_obj_id_t map_id = entry.get_map_id();
    nas_qos_map_type_t map_type = entry.get_type();

    nas_attr_id_t obj_id = qos_map_entry_obj_id(map_type);
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
            obj_id,
            cps_api_qualifier_TARGET);

    obj_id = qos_map_map_id_obj_id(map_type);
    cps_api_set_key_data(obj,
            obj_id,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    _fill_entry_key_data(obj, key, map_type);

    nas_qos_map_entry_value_t value = entry.get_value();
    for (auto attr_id: attr_set) {
        uint_t val;
        cps_api_object_ATTR_TYPE_t attr_type;
        if (attr_id == qos_map_entry_attr_1_obj_id(map_type)) {
            val = qos_map_entry_attr_1_get_fn(map_type)(value);
            attr_type = qos_map_get_attr_1_type(map_type);
            if (qos_map_add_int_by_type(obj, attr_id, val, attr_type) != true)
                return cps_api_ret_code_ERR;
        }

        if (attr_id == qos_map_entry_attr_2_obj_id(map_type)) {
            val = qos_map_entry_attr_2_get_fn(map_type)(value);
            attr_type = qos_map_get_attr_2_type(map_type);
            if (qos_map_add_int_by_type(obj, attr_id, val, attr_type) != true)
                return cps_api_ret_code_ERR;
        }
    }

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t _fill_matching_map_entries(
                                    cps_api_object_t ret_obj,
                                    nas_qos_map_entry_key_t search_key,
                                    map_entries *entries,
                                    nas_qos_map_type_t map_type)
{

    for (map_entries_iter_t it = entries->begin(); it != entries->end(); it++) {
        nas_qos_map_entry_key_t key = it->second.get_key();
        nas_qos_map_entry_value_t value = it->second.get_value();

        if (memcmp(&search_key, &key, sizeof(key)) == 0) {
            //get matched
            _fill_entry_key_data(ret_obj, key, map_type);

            nas_attr_id_t attr_id_1 = qos_map_entry_attr_1_obj_id(map_type);
            nas_attr_id_t attr_id_2 = qos_map_entry_attr_2_obj_id(map_type);

            uint_t val;
            cps_api_object_ATTR_TYPE_t attr_type;
            if (attr_id_1) {
                val = qos_map_entry_attr_1_get_fn(map_type)(value);
                attr_type = qos_map_get_attr_1_type(map_type);
                if (qos_map_add_int_by_type(ret_obj, attr_id_1, val, attr_type) != true)
                    return cps_api_ret_code_ERR;
            }

            if (attr_id_2) {
                val = qos_map_entry_attr_2_get_fn(map_type)(value);
                attr_type = qos_map_get_attr_2_type(map_type);
                if (qos_map_add_int_by_type(ret_obj, attr_id_2, val, attr_type) != true)
                    return cps_api_ret_code_ERR;
            }
        }
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t _fill_matching_map(
                                    cps_api_object_t ret_obj,
                                    map_entries *entries,
                                    nas_qos_map_type_t map_type)
{
    // fill in an embedded list of map entries as attributes.
    cps_api_attr_id_t   list_index = 0;
    for (map_entries_iter_t it = entries->begin(); it != entries->end(); it++) {
        nas_qos_map_entry_key_t key = it->second.get_key();
        nas_qos_map_entry_value_t value = it->second.get_value();

        nas_attr_id_t       parent_attr_id_list[3];
        parent_attr_id_list[0] = qos_map_entry_obj_id(map_type);
        parent_attr_id_list[1] = list_index;

        parent_attr_id_list[2] = qos_map_entry_key_1_obj_id(map_type);
        uint_t key1 = GET_KEY1(key);
        cps_api_object_ATTR_TYPE_t attr_type= qos_map_get_key_1_type(map_type);
        if (!qos_map_e_add_int_by_type (ret_obj,
                                   parent_attr_id_list,
                                   3,
                                   key1,
                                   attr_type)) {
            return cps_api_ret_code_ERR;
        }

        parent_attr_id_list[2] = qos_map_entry_key_2_obj_id(map_type);
        if (parent_attr_id_list[2] != 0) { //optional
            uint_t key2 = GET_KEY2(key);
            attr_type= qos_map_get_key_2_type(map_type);
            if (!qos_map_e_add_int_by_type (ret_obj,
                                       parent_attr_id_list,
                                       3,
                                       key2,
                                       attr_type)) {
                return cps_api_ret_code_ERR;
            }
        }


        parent_attr_id_list[2] = qos_map_entry_attr_1_obj_id(map_type);
        uint_t attr_val1 = qos_map_entry_attr_1_get_fn(map_type)(value);
        attr_type= qos_map_get_attr_1_type(map_type);
        if (!qos_map_e_add_int_by_type (ret_obj,
                                   parent_attr_id_list,
                                   3,
                                   attr_val1,
                                   attr_type)) {
            return cps_api_ret_code_ERR;
        }

        parent_attr_id_list[2] = qos_map_entry_attr_2_obj_id(map_type);
        if (parent_attr_id_list[2] != 0) { //optional
            uint_t attr_val2 = qos_map_entry_attr_2_get_fn(map_type)(value);
            attr_type= qos_map_get_attr_2_type(map_type);
            if (!qos_map_e_add_int_by_type (ret_obj,
                                       parent_attr_id_list,
                                       3,
                                       attr_val2,
                                       attr_type)) {
                return cps_api_ret_code_ERR;
            }
        }

        list_index++;
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_cps_validate_set_attr_list(
                                nas_qos_map_entry & entry,
                                nas_qos_map_type_t map_type)
{
    nas_attr_id_t attr_id_1 = qos_map_entry_attr_1_obj_id(map_type);
    nas_attr_id_t attr_id_2 = qos_map_entry_attr_2_obj_id(map_type);

    if (entry.dirty_attr_list().contains(attr_id_1) == false) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                "NAS map entry contains insufficient data");
        return NAS_QOS_E_MISSING_ATTR;
    }

    if (attr_id_2 != 0 &&
        entry.dirty_attr_list().contains(attr_id_1) == false) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                "NAS map entry contains insufficient data");
        return NAS_QOS_E_MISSING_ATTR;
    }

    return cps_api_ret_code_OK;
}


static void _fill_entry_key_data(cps_api_object_t obj,
        nas_qos_map_entry_key_t key,
        nas_qos_map_type_t map_type)
{
    nas_attr_id_t obj_id = qos_map_entry_key_1_obj_id(map_type);
    switch (map_type) {
    case NDI_QOS_MAP_DOT1P_TO_TC_COLOR:
    case NDI_QOS_MAP_DOT1P_TO_TC:
    case NDI_QOS_MAP_DOT1P_TO_COLOR:
    case NDI_QOS_MAP_DSCP_TO_TC_COLOR:
    case NDI_QOS_MAP_DSCP_TO_TC:
    case NDI_QOS_MAP_DSCP_TO_COLOR:
    case NDI_QOS_MAP_TC_TO_DSCP:
    case NDI_QOS_MAP_TC_TO_DOT1P:
    case NDI_QOS_MAP_TC_TO_PG:
    {
        uint8_t val8 = (uint8_t)key.any;
        cps_api_set_key_data(obj, obj_id,
            cps_api_object_ATTR_T_BIN,
            &val8, sizeof(uint8_t));
        break;
    }

    case NDI_QOS_MAP_TC_TO_QUEUE:
    {
        // For NDI_QOS_MAP_TC_TO_QUEUE, key is a complex key
        uint8_t tc = GET_KEY1(key);
        uint32_t queue_type = GET_KEY2(key);
        obj_id = qos_map_entry_key_1_obj_id(map_type);
        cps_api_set_key_data(obj,
                obj_id,
                cps_api_object_ATTR_T_BIN,
                &tc, sizeof(uint8_t));
        obj_id = qos_map_entry_key_2_obj_id(map_type);
        cps_api_set_key_data(obj,
                obj_id,
                cps_api_object_ATTR_T_U32,
                &queue_type, sizeof(uint32_t));
        break;
    }

    case NDI_QOS_MAP_TC_COLOR_TO_DOT1P:
    case NDI_QOS_MAP_TC_COLOR_TO_DSCP:
    {
        // For NDI_QOS_MAP_TC_TO_QUEUE, key is a complex key
        uint8_t tc = GET_KEY1(key);
        uint32_t color = GET_KEY2(key);
        obj_id = qos_map_entry_key_1_obj_id(map_type);
        cps_api_set_key_data(obj,
                obj_id,
                cps_api_object_ATTR_T_BIN,
                &tc, sizeof(uint8_t));
        obj_id = qos_map_entry_key_2_obj_id(map_type);
        cps_api_set_key_data(obj,
                obj_id,
                cps_api_object_ATTR_T_U32,
                &color, sizeof(uint32_t));
        break;
    }

    case NDI_QOS_MAP_PG_TO_PFC:
    {
        // complex key
        uint8_t pg = GET_KEY1(key);
        uint8_t prio = GET_KEY2(key);
        obj_id = qos_map_entry_key_1_obj_id(map_type);
        cps_api_set_key_data(obj,
                obj_id,
                cps_api_object_ATTR_T_BIN,
                &pg, sizeof(uint8_t));
        obj_id = qos_map_entry_key_2_obj_id(map_type);
        cps_api_set_key_data(obj,
                obj_id,
                cps_api_object_ATTR_T_BIN,
                &prio, sizeof(uint8_t));
        break;
    }

    case NDI_QOS_MAP_PFC_TO_QUEUE:
    {
        //complex key
        uint8_t prio = GET_KEY1(key);
        uint32_t queue_type = GET_KEY2(key);
        obj_id = qos_map_entry_key_1_obj_id(map_type);
        cps_api_set_key_data(obj,
                obj_id,
                cps_api_object_ATTR_T_BIN,
                &prio, sizeof(uint8_t));
        obj_id = qos_map_entry_key_2_obj_id(map_type);
        cps_api_set_key_data(obj,
                obj_id,
                cps_api_object_ATTR_T_U32,
                &queue_type, sizeof(uint32_t));
        break;
    }

    default:
        break;
    }

}



static bool qos_map_e_add_int_by_type (cps_api_object_t obj,
        cps_api_attr_id_t *id,  size_t id_size,
        uint val, cps_api_object_ATTR_TYPE_t type)
{
    switch (type) {
    case cps_api_object_ATTR_T_BIN:
    {
        uint8_t val8 = (uint8_t)val;
        return cps_api_object_e_add (obj,
                                   id, id_size,
                                   cps_api_object_ATTR_T_BIN,
                                   &val8,
                                   sizeof (uint8_t));
    }
    case cps_api_object_ATTR_T_U16:
    {
        uint16_t val16 = (uint16_t)val;
        return cps_api_object_e_add (obj,
                                   id, id_size,
                                   cps_api_object_ATTR_T_BIN,
                                   &val16,
                                   sizeof (uint16_t));

    }
    case cps_api_object_ATTR_T_U32:
    {
        uint32_t val32 = (uint32_t)val;
        return cps_api_object_e_add (obj,
                                   id, id_size,
                                   cps_api_object_ATTR_T_BIN,
                                   &val32,
                                   sizeof (uint32_t));

    }
    case cps_api_object_ATTR_T_U64:
    {
        uint64_t val64 = (uint64_t)val;
        return cps_api_object_e_add (obj,
                                   id, id_size,
                                   cps_api_object_ATTR_T_BIN,
                                   &val64,
                                   sizeof (uint64_t));

    }

    default:
        return false;
    }
}

static bool qos_map_add_int_by_type(cps_api_object_t obj,
        cps_api_attr_id_t id,
        uint_t val,
        cps_api_object_ATTR_TYPE_t type)
{
    switch (type) {
    case cps_api_object_ATTR_T_BIN:
    {
        uint8_t val8 = (uint8_t) val;
        return cps_api_object_attr_add(obj, id, &val8, sizeof(uint8_t));
    }
    case cps_api_object_ATTR_T_U16:
    {
        uint16_t val16 = (uint16_t) val;
        return cps_api_object_attr_add(obj, id, &val16, sizeof(uint16_t));
    }
    case cps_api_object_ATTR_T_U32:
    {
        uint32_t val32 = (uint32_t) val;
        return cps_api_object_attr_add(obj, id, &val32, sizeof(uint32_t));
    }
    case cps_api_object_ATTR_T_U64:
    {
        uint64_t val64 = (uint64_t) val;
        return cps_api_object_attr_add(obj, id, &val64, sizeof(uint64_t));
    }
    default:
        return false;
    }
}

