
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
 * \file   nas_qos_map_util.cpp
 * \brief  NAS QOS map util file for all map objects
 * \date   05-2015
 * \author
 */

#include "nas_qos_map.h"
#include "nas_qos_map_util.h"
#include "nas_qos_switch_list.h"
#include "nas_qos_switch.h"
#include "dell-base-qos.h"
#include "cps_api_object_key.h"
#include "cps_api_operation.h"
#include "cps_class_map.h"

#include <unordered_map>

typedef struct _attr_id_list_t {
    nas_attr_id_t    map_obj;
    nas_attr_id_t    entry_obj;
    nas_attr_id_t    map_id_obj;

    // keys
    nas_attr_id_t    entry_key_obj;
    cps_api_object_ATTR_TYPE_t key_1_type;
    nas_attr_id_t    entry_secondary_key_obj;
    cps_api_object_ATTR_TYPE_t key_2_type;

    // attributes
    nas_attr_id_t    entry_attr_obj_1;
    cps_api_object_ATTR_TYPE_t attr_1_type;
    _attr_parse_fn   entry_attr_obj_1_parse_fn;
    _attr_get_fn     entry_attr_obj_1_get_fn;

    nas_attr_id_t    entry_attr_obj_2;
    cps_api_object_ATTR_TYPE_t attr_2_type;
    _attr_parse_fn   entry_attr_obj_2_parse_fn;
    _attr_get_fn     entry_attr_obj_2_get_fn;
}_attr_id_list;


static void _attr_parse_dot1p_fn(
        cps_api_object_attr_t attr,
        nas_qos_map_entry  &entry)
{
    uint8_t val8 = *(uint8_t*)cps_api_object_attr_data_bin(attr);
    entry.set_value_dot1p(val8);
}

static void _attr_parse_dscp_fn(
        cps_api_object_attr_t attr,
        nas_qos_map_entry  &entry)
{
    uint8_t val8 = *(uint8_t*)cps_api_object_attr_data_bin(attr);
    entry.set_value_dscp(val8);
}

static void _attr_parse_tc_fn(
        cps_api_object_attr_t attr,
        nas_qos_map_entry  &entry)
{
    uint8_t val8 = *(uint8_t *)cps_api_object_attr_data_bin(attr);
    entry.set_value_tc(val8);
}

static void _attr_parse_color_fn(
        cps_api_object_attr_t attr,
        nas_qos_map_entry  &entry)
{
    uint32_t val32 = cps_api_object_attr_data_u32(attr);
    entry.set_value_color((BASE_QOS_PACKET_COLOR_t)val32);
}

static void _attr_parse_queue_number_fn(
        cps_api_object_attr_t attr,
        nas_qos_map_entry  &entry)
{
    uint32_t val32 = cps_api_object_attr_data_u32(attr);
    entry.set_value_queue_num(val32);
}

static void _attr_parse_pg_fn(
        cps_api_object_attr_t attr,
        nas_qos_map_entry  &entry)
{
    uint8_t val8 = *(uint8_t *)cps_api_object_attr_data_bin(attr);
    entry.set_value_pg(val8);
}

static uint_t _attr_get_dot1p_fn(
        nas_qos_map_entry_value_t &value)
{
    return value.dot1p;
}

static uint_t _attr_get_dscp_fn(
        nas_qos_map_entry_value_t &value)
{
    return value.dscp;
}

static uint_t _attr_get_tc_fn(
        nas_qos_map_entry_value_t &value)
{
    return value.tc;
}

static uint_t _attr_get_color_fn(
        nas_qos_map_entry_value_t &value)
{
    return value.color;
}

static uint_t _attr_get_queue_number_fn(
        nas_qos_map_entry_value_t &value)
{
    return value.queue_num;
}

static uint_t _attr_get_pg_fn(
        nas_qos_map_entry_value_t &value)
{
    return value.pg;
}



static const
    std::unordered_map<nas_qos_map_type_t, _attr_id_list, std::hash<int>>
        _yang_qos_map_obj_map = {
                {NDI_QOS_MAP_DOT1P_TO_TC,

                    {BASE_QOS_DOT1P_TO_TC_MAP_OBJ,
                     BASE_QOS_DOT1P_TO_TC_MAP_ENTRY,
                     BASE_QOS_DOT1P_TO_TC_MAP_ID,
                     //keys
                     BASE_QOS_DOT1P_TO_TC_MAP_ENTRY_DOT1P,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     //attr
                     BASE_QOS_DOT1P_TO_TC_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_tc_fn,
                     _attr_get_tc_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    },
                },

                {NDI_QOS_MAP_DOT1P_TO_COLOR,
                    {BASE_QOS_DOT1P_TO_COLOR_MAP_OBJ,
                     BASE_QOS_DOT1P_TO_COLOR_MAP_ENTRY,
                     BASE_QOS_DOT1P_TO_COLOR_MAP_ID,
                     //keys
                     BASE_QOS_DOT1P_TO_COLOR_MAP_ENTRY_DOT1P,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,

                     //attr
                     BASE_QOS_DOT1P_TO_COLOR_MAP_ENTRY_COLOR,
                     cps_api_object_ATTR_T_U32,
                     _attr_parse_color_fn,
                     _attr_get_color_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_DOT1P_TO_TC_COLOR,
                    {BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ,
                     BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY,
                     BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID,
                     //keys
                     BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_DOT1P,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,

                     //attr
                     BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_tc_fn,
                     _attr_get_tc_fn,
                     BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_COLOR,
                     cps_api_object_ATTR_T_U32,
                     _attr_parse_color_fn,
                     _attr_get_color_fn,
                    }
                },
                {NDI_QOS_MAP_DSCP_TO_TC,
                    {BASE_QOS_DSCP_TO_TC_MAP_OBJ,
                     BASE_QOS_DSCP_TO_TC_MAP_ENTRY,
                     BASE_QOS_DSCP_TO_TC_MAP_ID,
                     //keys
                     BASE_QOS_DSCP_TO_TC_MAP_ENTRY_DSCP,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,

                     //attr
                     BASE_QOS_DSCP_TO_TC_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_tc_fn,
                     _attr_get_tc_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_DSCP_TO_COLOR,
                    {BASE_QOS_DSCP_TO_COLOR_MAP_OBJ,
                     BASE_QOS_DSCP_TO_COLOR_MAP_ENTRY,
                     BASE_QOS_DSCP_TO_COLOR_MAP_ID,
                     //keys
                     BASE_QOS_DSCP_TO_COLOR_MAP_ENTRY_DSCP,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,

                     //attr
                     BASE_QOS_DSCP_TO_COLOR_MAP_ENTRY_COLOR,
                     cps_api_object_ATTR_T_U32,
                     _attr_parse_color_fn,
                     _attr_get_color_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_DSCP_TO_TC_COLOR,
                    {BASE_QOS_DSCP_TO_TC_COLOR_MAP_OBJ,
                     BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY,
                     BASE_QOS_DSCP_TO_TC_COLOR_MAP_ID,
                     //keys
                     BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_DSCP,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,

                     //attr
                     BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_tc_fn,
                     _attr_get_tc_fn,
                     BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_COLOR,
                     cps_api_object_ATTR_T_U32,
                     _attr_parse_color_fn,
                     _attr_get_color_fn,
                    }
                },
                {NDI_QOS_MAP_TC_TO_DOT1P,
                    {BASE_QOS_TC_TO_DOT1P_MAP_OBJ,
                     BASE_QOS_TC_TO_DOT1P_MAP_ENTRY,
                     BASE_QOS_TC_TO_DOT1P_MAP_ID,
                     //keys
                     BASE_QOS_TC_TO_DOT1P_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,

                     //attr
                     BASE_QOS_TC_TO_DOT1P_MAP_ENTRY_DOT1P,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_dot1p_fn,
                     _attr_get_dot1p_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_TC_TO_DSCP,
                    {BASE_QOS_TC_TO_DSCP_MAP_OBJ,
                     BASE_QOS_TC_TO_DSCP_MAP_ENTRY,
                     BASE_QOS_TC_TO_DSCP_MAP_ID,
                     //keys
                     BASE_QOS_TC_TO_DSCP_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,

                     //attr
                     BASE_QOS_TC_TO_DSCP_MAP_ENTRY_DSCP,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_dscp_fn,
                     _attr_get_dscp_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_TC_COLOR_TO_DOT1P,
                    {BASE_QOS_TC_COLOR_TO_DOT1P_MAP_OBJ,
                     BASE_QOS_TC_COLOR_TO_DOT1P_MAP_ENTRY,
                     BASE_QOS_TC_COLOR_TO_DOT1P_MAP_ID,
                     //keys
                     BASE_QOS_TC_COLOR_TO_DOT1P_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     BASE_QOS_TC_COLOR_TO_DOT1P_MAP_ENTRY_COLOR,
                     cps_api_object_ATTR_T_U32,
                     //attr
                     BASE_QOS_TC_COLOR_TO_DOT1P_MAP_ENTRY_DOT1P,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_dot1p_fn,
                     _attr_get_dot1p_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_TC_COLOR_TO_DSCP,
                    {BASE_QOS_TC_COLOR_TO_DSCP_MAP_OBJ,
                     BASE_QOS_TC_COLOR_TO_DSCP_MAP_ENTRY,
                     BASE_QOS_TC_COLOR_TO_DSCP_MAP_ID,
                     //keys
                     BASE_QOS_TC_COLOR_TO_DSCP_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     BASE_QOS_TC_COLOR_TO_DSCP_MAP_ENTRY_COLOR,
                     cps_api_object_ATTR_T_U32,
                     //attr
                     BASE_QOS_TC_COLOR_TO_DSCP_MAP_ENTRY_DSCP,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_dscp_fn,
                     _attr_get_dscp_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_TC_TO_QUEUE,
                    {BASE_QOS_TC_TO_QUEUE_MAP_OBJ,
                     BASE_QOS_TC_TO_QUEUE_MAP_ENTRY,
                     BASE_QOS_TC_TO_QUEUE_MAP_ID,
                     //keys
                     BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TYPE,
                     cps_api_object_ATTR_T_U32,
                     //attr
                     BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_QUEUE_NUMBER,
                     cps_api_object_ATTR_T_U32,
                     _attr_parse_queue_number_fn,
                     _attr_get_queue_number_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_TC_TO_PG,
                    {BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_OBJ,
                     BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_ENTRY,
                     BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_ID,
                     //keys
                     BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_ENTRY_TC,
                     cps_api_object_ATTR_T_BIN,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     //attr
                     BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_ENTRY_PRIORITY_GROUP,
                     cps_api_object_ATTR_T_BIN,
                     _attr_parse_pg_fn,
                     _attr_get_pg_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_PG_TO_PFC,
                    {BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_OBJ,
                     BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_ENTRY,
                     BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_ID,
                     //keys
                     BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_ENTRY_PRIORITY_GROUP,
                     cps_api_object_ATTR_T_BIN,
                     BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_ENTRY_PFC_PRIORITY,
                     cps_api_object_ATTR_T_BIN,
                     //attr
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },
                {NDI_QOS_MAP_PFC_TO_QUEUE,
                    {BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_OBJ,
                     BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_ENTRY,
                     BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_ID,
                     //keys
                     BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_ENTRY_PFC_PRIORITY,
                     cps_api_object_ATTR_T_BIN,
                     BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_ENTRY_TYPE,
                     cps_api_object_ATTR_T_U32,
                     //attr
                     BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_ENTRY_QUEUE_NUMBER,
                     cps_api_object_ATTR_T_U32,
                     _attr_parse_queue_number_fn,
                     _attr_get_queue_number_fn,
                     0, (cps_api_object_ATTR_TYPE_t)0,
                     NULL,
                     NULL,
                    }
                },

        };

nas_attr_id_t qos_map_obj_id(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).map_obj;
}

nas_attr_id_t qos_map_entry_obj_id(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_obj;
}


nas_attr_id_t qos_map_map_id_obj_id(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).map_id_obj;
}

nas_attr_id_t qos_map_entry_key_1_obj_id(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_key_obj;
}

nas_attr_id_t qos_map_entry_key_2_obj_id(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_secondary_key_obj;
}

nas_attr_id_t qos_map_entry_attr_1_obj_id(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_attr_obj_1;
}

nas_attr_id_t qos_map_entry_attr_2_obj_id(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_attr_obj_2;
}

_attr_get_fn qos_map_entry_attr_1_get_fn(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_attr_obj_1_get_fn;
}

_attr_get_fn qos_map_entry_attr_2_get_fn(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_attr_obj_2_get_fn;
}

_attr_parse_fn qos_map_entry_attr_1_parse_fn(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_attr_obj_1_parse_fn;
}

_attr_parse_fn qos_map_entry_attr_2_parse_fn(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).entry_attr_obj_2_parse_fn;
}

cps_api_object_ATTR_TYPE_t qos_map_get_key_1_type(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).key_1_type;
}
cps_api_object_ATTR_TYPE_t qos_map_get_key_2_type(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).key_2_type;
}
cps_api_object_ATTR_TYPE_t qos_map_get_attr_1_type(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).attr_1_type;
}
cps_api_object_ATTR_TYPE_t qos_map_get_attr_2_type(nas_qos_map_type_t type)
{
    return _yang_qos_map_obj_map.at(type).attr_2_type;
}

nas_qos_map * nas_qos_cps_get_map(uint_t switch_id, nas_obj_id_t map_id)
{

    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NULL;

    nas_qos_map *map_p = p_switch->get_map(map_id);

    return map_p;
}

cps_api_return_code_t  qos_map_get_switch_id(cps_api_object_t obj,
                                             nas_qos_map_type_t type,
                                             uint_t &switch_id)
{
    switch_id = 0;

    return cps_api_ret_code_OK;

}

cps_api_return_code_t  qos_map_get_key(cps_api_object_t obj,
                                        nas_qos_map_type_t type,
                                        uint_t &switch_id,
                                        nas_obj_id_t &map_id)
{
    if (qos_map_get_switch_id(obj, type, switch_id) != cps_api_ret_code_OK)
        return cps_api_ret_code_ERR;

    try {
        nas_attr_id_t attr_id = qos_map_map_id_obj_id(type);

        cps_api_object_attr_t map_attr;
        map_attr = cps_api_get_key_data(obj, attr_id);
        if (map_attr == NULL) {
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "map id not specified\n");
            return cps_api_ret_code_ERR;
        }
        map_id = cps_api_object_attr_data_u64(map_attr);

    } catch (std::out_of_range e) {
        EV_LOG_ERR(ev_log_t_QOS, 3, "NAS-QOS",
                    "attribute id for map_id is not mapped correctly");
        return cps_api_ret_code_ERR;
    }


    return cps_api_ret_code_OK;
}

cps_api_return_code_t  qos_map_entry_get_key(cps_api_object_t obj,
                                                nas_qos_map_type_t type,
                                                uint_t &switch_id,
                                                nas_obj_id_t &map_id,
                                                nas_qos_map_entry_key_t &key)
{
    if (qos_map_get_key(obj, type, switch_id, map_id) != cps_api_ret_code_OK)
        return cps_api_ret_code_ERR;

    try {
        nas_attr_id_t attr_id = qos_map_entry_key_1_obj_id(type);

        cps_api_object_attr_t entry_key_attr = cps_api_get_key_data(obj, attr_id);
        if (entry_key_attr == NULL) {
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "map entry key not specified\n");
            return cps_api_ret_code_ERR;
        }

        switch (type) {
        case NDI_QOS_MAP_DOT1P_TO_TC:
        case NDI_QOS_MAP_DOT1P_TO_COLOR:
        case NDI_QOS_MAP_DOT1P_TO_TC_COLOR:
        case NDI_QOS_MAP_DSCP_TO_TC:
        case NDI_QOS_MAP_DSCP_TO_COLOR:
        case NDI_QOS_MAP_DSCP_TO_TC_COLOR:
        case NDI_QOS_MAP_TC_TO_DSCP:
        case NDI_QOS_MAP_TC_TO_DOT1P:
        case NDI_QOS_MAP_TC_TO_PG:
            key.any = *(uint8_t *)cps_api_object_attr_data_bin(entry_key_attr);
            break;

        case NDI_QOS_MAP_TC_TO_QUEUE:
        case NDI_QOS_MAP_TC_COLOR_TO_DOT1P:
        case NDI_QOS_MAP_TC_COLOR_TO_DSCP:
        {
            uint8_t tc = *(uint8_t *)cps_api_object_attr_data_bin(entry_key_attr);

            nas_attr_id_t attr2_id = qos_map_entry_key_2_obj_id(type);

            cps_api_object_attr_t entry_key2_attr = cps_api_get_key_data(obj, attr2_id);
            if (entry_key2_attr == NULL) {
                EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "map entry key 2 not specified\n");
                return cps_api_ret_code_ERR;
            }

            if (type == NDI_QOS_MAP_TC_TO_QUEUE) {
                uint32_t type_val = cps_api_object_attr_data_u32(entry_key2_attr);
                key.any = MAKE_KEY(tc, type_val);
            }
            else {
                uint32_t color = cps_api_object_attr_data_u32(entry_key2_attr);
                key.any = MAKE_KEY(tc, color);
            }

            break;
        }

        case NDI_QOS_MAP_PG_TO_PFC:
        {
            uint8_t pg = *(uint8_t *)cps_api_object_attr_data_bin(entry_key_attr);

            nas_attr_id_t attr2_id = qos_map_entry_key_2_obj_id(type);

            cps_api_object_attr_t entry_key2_attr = cps_api_get_key_data(obj, attr2_id);
            if (entry_key2_attr == NULL) {
                EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "map entry key 2 not specified\n");
                return cps_api_ret_code_ERR;
            }

            uint8_t prio = *(uint8_t *)cps_api_object_attr_data_bin(entry_key2_attr);
            key.any = MAKE_KEY(pg, prio);
            break;
        }

        case NDI_QOS_MAP_PFC_TO_QUEUE:
        {
            uint8_t prio = *(uint8_t *)cps_api_object_attr_data_bin(entry_key_attr);

            nas_attr_id_t attr2_id = qos_map_entry_key_2_obj_id(type);

            cps_api_object_attr_t entry_key2_attr = cps_api_get_key_data(obj, attr2_id);
            if (entry_key2_attr == NULL) {
                EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "map entry key 2 not specified\n");
                return cps_api_ret_code_ERR;
            }

            uint32_t type_val = cps_api_object_attr_data_u32(entry_key2_attr);
            key.any = MAKE_KEY(prio, type_val);
            break;
        }

        default:
            return cps_api_ret_code_ERR;
        }

    } catch (std::out_of_range e) {
        EV_LOG_ERR(ev_log_t_QOS, 3, "NAS-QOS",
                    "attribute id for map entry key is not mapped correctly");
        return cps_api_ret_code_ERR;
    }

    return cps_api_ret_code_OK;
}

cps_api_return_code_t qos_map_key_check(cps_api_object_t obj,
                                        nas_qos_map_type_t type,
                                        bool *is_map_entry)
{
    try {
        cps_api_key_t map_key, entry_key;

        nas_obj_id_t obj_id = qos_map_obj_id(type);
        cps_api_key_from_attr_with_qual(&map_key,
                obj_id,
                cps_api_qualifier_TARGET);

        if (cps_api_key_matches (&map_key, cps_api_object_key(obj), true) == 0) {
            *is_map_entry = false;
            return cps_api_ret_code_OK;
        }


        obj_id = qos_map_entry_obj_id(type);
        cps_api_key_from_attr_with_qual(&entry_key,
                obj_id,
                cps_api_qualifier_TARGET);

        if (cps_api_key_matches (&entry_key, cps_api_object_key(obj), true) == 0) {
            *is_map_entry = true;
            return cps_api_ret_code_OK;
        }

    } catch (std::out_of_range e) {
        EV_LOG_ERR(ev_log_t_QOS, 3, "NAS-QOS",
                    "object id or entry object id is not mapped correctly");
        return cps_api_ret_code_ERR;
    }

    return cps_api_ret_code_ERR;

}


