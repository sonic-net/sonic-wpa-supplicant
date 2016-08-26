
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
 * \file   nas_qos_cps_map.cpp
 * \brief  NAS qos map related CPS API routines
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
#include "nas_qos_cps_map.h"


static std_mutex_lock_create_static_init_rec(map_mutex);

static std::unordered_map<nas_attr_id_t, nas_qos_map_type_t, std::hash<int>>
        _yang_obj_id_to_nas_type_map = {
                {BASE_QOS_DOT1P_TO_TC_MAP_OBJ,
                    NDI_QOS_MAP_DOT1P_TO_TC,
                },
                {BASE_QOS_DOT1P_TO_COLOR_MAP_OBJ,
                    NDI_QOS_MAP_DOT1P_TO_COLOR,
                },
                {BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ,
                    NDI_QOS_MAP_DOT1P_TO_TC_COLOR,
                },
                {BASE_QOS_DSCP_TO_TC_MAP_OBJ,
                    NDI_QOS_MAP_DSCP_TO_TC,
                },
                {BASE_QOS_DSCP_TO_COLOR_MAP_OBJ,
                    NDI_QOS_MAP_DSCP_TO_COLOR,
                },
                {BASE_QOS_DSCP_TO_TC_COLOR_MAP_OBJ,
                    NDI_QOS_MAP_DSCP_TO_TC_COLOR,
                },
                {BASE_QOS_TC_TO_DSCP_MAP_OBJ,
                    NDI_QOS_MAP_TC_TO_DSCP,
                },
                {BASE_QOS_TC_TO_DOT1P_MAP_OBJ,
                    NDI_QOS_MAP_TC_TO_DOT1P,
                },
                {BASE_QOS_TC_TO_QUEUE_MAP_OBJ,
                    NDI_QOS_MAP_TC_TO_QUEUE,
                },
                {BASE_QOS_TC_COLOR_TO_DOT1P_MAP_OBJ,
                    NDI_QOS_MAP_TC_COLOR_TO_DOT1P,
                },
                {BASE_QOS_TC_COLOR_TO_DSCP_MAP_OBJ,
                    NDI_QOS_MAP_TC_COLOR_TO_DSCP,
                },
                {BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_OBJ,
                    NDI_QOS_MAP_TC_TO_PG,
                },
                {BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_OBJ,
                    NDI_QOS_MAP_PG_TO_PFC,
                },
                {BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_OBJ,
                    NDI_QOS_MAP_PFC_TO_QUEUE,
                },
        };

/**
  * This function provides NAS-QoS Map CPS API write function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_map_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_return_code_t rc = cps_api_ret_code_ERR;

    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    if (obj == NULL)
        return cps_api_ret_code_ERR;

    std_mutex_simple_lock_guard p_m(&map_mutex);

    /* Parse sub-category */
    uint_t subcat_id = cps_api_key_get_subcat(cps_api_object_key(obj));
    try {
        rc = nas_qos_cps_api_map_write_type(context, param, ix,
                _yang_obj_id_to_nas_type_map.at(subcat_id));

    }
    catch (...) {
        return NAS_QOS_E_FAIL;
    }

    return rc;
}


/**
  * This function provides NAS-QoS Map CPS API read function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_map_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    cps_api_return_code_t rc = cps_api_ret_code_ERR;

    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    if (obj == NULL)
        return cps_api_ret_code_ERR;

    std_mutex_simple_lock_guard p_m(&map_mutex);

    /* Parse sub-category */
    uint_t subcat_id = cps_api_key_get_subcat(cps_api_object_key(obj));
    try {
        rc = nas_qos_cps_api_map_read_type(context, param, ix,
                _yang_obj_id_to_nas_type_map.at(subcat_id));

    }
    catch (...) {
        return NAS_QOS_E_FAIL;
    }


    return rc;
}


/**
  * This function provides NAS-QoS Map CPS API rollback function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_map_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_return_code_t rc = cps_api_ret_code_ERR;

    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    if (obj == NULL)
        return cps_api_ret_code_ERR;

    std_mutex_simple_lock_guard p_m(&map_mutex);

    /* Parse sub-category */
    uint_t subcat_id = cps_api_key_get_subcat(cps_api_object_key(obj));
    try {
        rc = nas_qos_cps_api_map_rollback_type(context, param, ix,
                _yang_obj_id_to_nas_type_map.at(subcat_id));

    }
    catch (...) {
        return NAS_QOS_E_FAIL;
    }

    return rc;
}

