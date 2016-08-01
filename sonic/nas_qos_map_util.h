
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
 * \file   nas_qos_map_util.h
 * \brief  NAS QOS map object
 * \date   05-2015
 * \author
 */

#ifndef _NAS_QOS_MAP_UTIL_H_
#define _NAS_QOS_MAP_UTIL_H_

#include "nas_qos_map.h"
#include "nas_types.h"


typedef uint_t (*_attr_get_fn) (nas_qos_map_entry_value_t &value);

typedef void (*_attr_parse_fn) (
        cps_api_object_attr_t attr,
        nas_qos_map_entry  &entry);



nas_qos_map * nas_qos_cps_get_map(uint_t switch_id, nas_obj_id_t map_id);

cps_api_return_code_t  qos_map_get_switch_id(cps_api_object_t obj,
                                             nas_qos_map_type_t type,
                                             uint_t &switch_id);

cps_api_return_code_t  qos_map_get_key(cps_api_object_t obj,
                                        nas_qos_map_type_t type,
                                        uint_t &switch_id,
                                        nas_obj_id_t &map_id);

cps_api_return_code_t  qos_map_entry_get_key(cps_api_object_t obj,
                                            nas_qos_map_type_t type,
                                            uint_t &switch_id,
                                            nas_obj_id_t &map_id,
                                            nas_qos_map_entry_key_t &key);

cps_api_return_code_t qos_map_key_check(cps_api_object_t obj,
                                        nas_qos_map_type_t type,
                                        bool *is_map_entry);

nas_attr_id_t qos_map_obj_id(nas_qos_map_type_t type);
nas_attr_id_t qos_map_entry_obj_id(nas_qos_map_type_t type);

nas_attr_id_t qos_map_switch_id_obj_id(nas_qos_map_type_t type);
nas_attr_id_t qos_map_map_id_obj_id(nas_qos_map_type_t type);

nas_attr_id_t qos_map_entry_key_1_obj_id(nas_qos_map_type_t type);
nas_attr_id_t qos_map_entry_key_2_obj_id(nas_qos_map_type_t type);
nas_attr_id_t qos_map_entry_attr_1_obj_id(nas_qos_map_type_t type);
nas_attr_id_t qos_map_entry_attr_2_obj_id(nas_qos_map_type_t type);

_attr_get_fn  qos_map_entry_attr_1_get_fn(nas_qos_map_type_t type);
_attr_get_fn  qos_map_entry_attr_2_get_fn(nas_qos_map_type_t type);
_attr_parse_fn qos_map_entry_attr_1_parse_fn(nas_qos_map_type_t type);
_attr_parse_fn qos_map_entry_attr_2_parse_fn(nas_qos_map_type_t type);

cps_api_object_ATTR_TYPE_t qos_map_get_key_1_type(nas_qos_map_type_t type);
cps_api_object_ATTR_TYPE_t qos_map_get_key_2_type(nas_qos_map_type_t type);
cps_api_object_ATTR_TYPE_t qos_map_get_attr_1_type(nas_qos_map_type_t type);
cps_api_object_ATTR_TYPE_t qos_map_get_attr_2_type(nas_qos_map_type_t type);

#endif
