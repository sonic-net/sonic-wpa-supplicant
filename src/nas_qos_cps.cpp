
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
 * \file   nas_qos_cps.cpp
 * \brief  NAS qos CPS API routines
 * \date   02-2015
 * \author
 */

#include "cps_api_events.h"
#include "cps_api_operation.h"

#include "event_log_types.h"
#include "event_log.h"
#include "std_error_codes.h"

#include "nas_qos_common.h"
#include "nas_qos_cps.h"
#include "cps_api_key.h"
#include "dell-base-qos.h"

#include "nas_qos_cps_policer.h"
#include "nas_qos_cps_queue.h"
#include "nas_qos_cps_wred.h"
#include "nas_qos_cps_scheduler.h"
#include "nas_qos_cps_scheduler_group.h"
#include "nas_qos_cps_map.h"
#include "nas_qos_cps_port_ingress.h"
#include "nas_qos_cps_port_egress.h"
#include "nas_qos_cps_buffer_pool.h"
#include "nas_qos_cps_buffer_profile.h"
#include "nas_qos_cps_priority_group.h"

/**
 * This function provides NAS-QoS CPS API read function
 * @Param    Standard CPS API params
 * @Return   Standard Error Code
 */
cps_api_return_code_t nas_qos_cps_api_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{
    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "nas_qos_cps_api_read...");

    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    if (obj == NULL)
        return cps_api_ret_code_ERR;

    /* Parse sub-category */
    uint_t subcat_id = cps_api_key_get_subcat(cps_api_object_key(obj));
    switch (subcat_id ) {
    case BASE_QOS_METER_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_METER_OBJ");
        return nas_qos_cps_api_policer_read(context, param, ix);

    case BASE_QOS_QUEUE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_QUEUE_OBJ");
        return nas_qos_cps_api_queue_read(context, param, ix);

    case BASE_QOS_QUEUE_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_QUEUE_STAT_OBJ");
        return nas_qos_cps_api_queue_stat_read(context, param, ix);

    case BASE_QOS_WRED_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_WRED_PROFILE_OBJ");
        return nas_qos_cps_api_wred_read(context, param, ix);

    case BASE_QOS_SCHEDULER_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_SCHEDULER_PROFILE_OBJ");
        return nas_qos_cps_api_scheduler_read(context, param, ix);

    case BASE_QOS_SCHEDULER_GROUP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_SCHEDULER_GROUP_OBJ");
        return nas_qos_cps_api_scheduler_group_read(context, param, ix);

    case BASE_QOS_DOT1P_TO_TC_MAP_OBJ:
    case BASE_QOS_DOT1P_TO_COLOR_MAP_OBJ:
    case BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ:
    case BASE_QOS_DSCP_TO_TC_MAP_OBJ:
    case BASE_QOS_DSCP_TO_COLOR_MAP_OBJ:
    case BASE_QOS_DSCP_TO_TC_COLOR_MAP_OBJ:
    case BASE_QOS_TC_TO_DOT1P_MAP_OBJ:
    case BASE_QOS_TC_TO_DSCP_MAP_OBJ:
    case BASE_QOS_TC_COLOR_TO_DOT1P_MAP_OBJ:
    case BASE_QOS_TC_COLOR_TO_DSCP_MAP_OBJ:
    case BASE_QOS_TC_TO_QUEUE_MAP_OBJ:
    case BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_OBJ:
    case BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_OBJ:
    case BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_*_MAP_OBJ");
        return nas_qos_cps_api_map_read(context, param, ix);

    case BASE_QOS_PORT_INGRESS_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PORT_INGRESS_OBJ");
        return nas_qos_cps_api_port_ingress_read(context, param, ix);

    case BASE_QOS_PORT_EGRESS_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PORT_EGRESS_OBJ");
        return nas_qos_cps_api_port_egress_read(context, param, ix);

    case BASE_QOS_BUFFER_POOL_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_POOL_OBJ");
        return nas_qos_cps_api_buffer_pool_read(context, param, ix);

    case BASE_QOS_BUFFER_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_PROFILE_OBJ");
        return nas_qos_cps_api_buffer_profile_read(context, param, ix);

    case BASE_QOS_PRIORITY_GROUP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PRIORITY_GROUP_OBJ");
        return nas_qos_cps_api_priority_group_read(context, param, ix);

    case BASE_QOS_BUFFER_POOL_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_POOL_STAT_OBJ");
        return nas_qos_cps_api_buffer_pool_stat_read(context, param, ix);

    case BASE_QOS_PRIORITY_GROUP_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PRIORITY_GROUP_STAT_OBJ");
        return nas_qos_cps_api_priority_group_stat_read(context, param, ix);


    default:
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_WARNING, "QOS", "Unknown QoS subcat");
        return NAS_QOS_E_UNSUPPORTED;

    }
}

/**
 * This function provides NAS-QoS CPS API write function
 * @Param      Standard CPS API params
 * @Return   Standard Error Code
 */
cps_api_return_code_t nas_qos_cps_api_write (void * context,
                                             cps_api_transaction_params_t * param,
                                             size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    if (obj == NULL)
        return cps_api_ret_code_ERR;

    /* Parse sub-category */
    uint_t subcat_id = cps_api_key_get_subcat(cps_api_object_key(obj));
    switch (subcat_id ) {
    case BASE_QOS_METER_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_METER_OBJ");
        return nas_qos_cps_api_policer_write(context, param, ix);

    case BASE_QOS_QUEUE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_QUEUE_OBJ");
        return nas_qos_cps_api_queue_write(context, param, ix);

    case BASE_QOS_QUEUE_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_QUEUE_STAT_OBJ");
        return nas_qos_cps_api_queue_stat_clear(context, param, ix);

    case BASE_QOS_WRED_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_WRED_PROFILE_OBJ");
        return nas_qos_cps_api_wred_write(context, param, ix);

    case BASE_QOS_SCHEDULER_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_SCHEDULER_PROFILE_OBJ");
        return nas_qos_cps_api_scheduler_write(context, param, ix);


    case BASE_QOS_SCHEDULER_GROUP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_SCHEDULER_GROUP_OBJ");
        return nas_qos_cps_api_scheduler_group_write(context, param, ix);

    case BASE_QOS_DOT1P_TO_TC_MAP_OBJ:
    case BASE_QOS_DOT1P_TO_COLOR_MAP_OBJ:
    case BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ:
    case BASE_QOS_DSCP_TO_TC_MAP_OBJ:
    case BASE_QOS_DSCP_TO_COLOR_MAP_OBJ:
    case BASE_QOS_DSCP_TO_TC_COLOR_MAP_OBJ:
    case BASE_QOS_TC_TO_DOT1P_MAP_OBJ:
    case BASE_QOS_TC_TO_DSCP_MAP_OBJ:
    case BASE_QOS_TC_COLOR_TO_DOT1P_MAP_OBJ:
    case BASE_QOS_TC_COLOR_TO_DSCP_MAP_OBJ:
    case BASE_QOS_TC_TO_QUEUE_MAP_OBJ:
    case BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_OBJ:
    case BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_OBJ:
    case BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_*_MAP_OBJ");
        return nas_qos_cps_api_map_write(context, param, ix);

    case BASE_QOS_PORT_INGRESS_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PORT_INGRESS_OBJ");
        return nas_qos_cps_api_port_ingress_write(context, param, ix);

    case BASE_QOS_PORT_EGRESS_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PORT_EGRESS_OBJ");
        return nas_qos_cps_api_port_egress_write(context, param, ix);

    case BASE_QOS_BUFFER_POOL_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_POOL_OBJ");
        return nas_qos_cps_api_buffer_pool_write(context, param, ix);

    case BASE_QOS_BUFFER_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_PROFILE_OBJ");
        return nas_qos_cps_api_buffer_profile_write(context, param, ix);

    case BASE_QOS_PRIORITY_GROUP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PRIORITY_GROUP_OBJ");
        return nas_qos_cps_api_priority_group_write(context, param, ix);

    case BASE_QOS_BUFFER_POOL_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_POOL_STAT_OBJ, not supported");
        return NAS_QOS_E_UNSUPPORTED;

    case BASE_QOS_PRIORITY_GROUP_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PRIORITY_GROUP_STAT_OBJ");
        return nas_qos_cps_api_priority_group_stat_clear(context, param, ix);

    default:
        return NAS_QOS_E_UNSUPPORTED;

    }

}

/**
  * This function provides NAS-QoS CPS API rollback function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_rollback (void * context,
                                                cps_api_transaction_params_t * param,
                                                size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    if (obj == NULL)
        return cps_api_ret_code_ERR;

    /* Parse sub-category */
    uint_t subcat_id = cps_api_key_get_subcat(cps_api_object_key(obj));
    switch (subcat_id ) {
    case BASE_QOS_METER_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_METER_OBJ");
        return nas_qos_cps_api_policer_rollback(context, param, ix);

    case BASE_QOS_QUEUE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_QUEUE_OBJ");
        return nas_qos_cps_api_queue_rollback(context, param, ix);

    case BASE_QOS_QUEUE_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_QUEUE_STAT_OBJ Rollback is not supported");
        return NAS_QOS_E_UNSUPPORTED;

    case BASE_QOS_WRED_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_WRED_PROFILE_OBJ");
        return nas_qos_cps_api_wred_rollback(context, param, ix);

    case BASE_QOS_SCHEDULER_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_SCHEDULER_PROFILE_OBJ");
        return nas_qos_cps_api_scheduler_rollback(context, param, ix);

    case BASE_QOS_SCHEDULER_GROUP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_SCHEDULER_GROUP_OBJ");
        return nas_qos_cps_api_scheduler_group_rollback(context, param, ix);

    case BASE_QOS_DOT1P_TO_TC_MAP_OBJ:
    case BASE_QOS_DOT1P_TO_COLOR_MAP_OBJ:
    case BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ:
    case BASE_QOS_DSCP_TO_TC_MAP_OBJ:
    case BASE_QOS_DSCP_TO_COLOR_MAP_OBJ:
    case BASE_QOS_DSCP_TO_TC_COLOR_MAP_OBJ:
    case BASE_QOS_TC_TO_DOT1P_MAP_OBJ:
    case BASE_QOS_TC_TO_DSCP_MAP_OBJ:
    case BASE_QOS_TC_COLOR_TO_DOT1P_MAP_OBJ:
    case BASE_QOS_TC_COLOR_TO_DSCP_MAP_OBJ:
    case BASE_QOS_TC_TO_QUEUE_MAP_OBJ:
    case BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_OBJ:
    case BASE_QOS_PRIORITY_GROUP_TO_PFC_PRIORITY_MAP_OBJ:
    case BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "BASE_QOS_*_MAP_OBJ");
        return nas_qos_cps_api_map_rollback(context, param, ix);

    case BASE_QOS_PORT_INGRESS_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PORT_INGRESS_OBJ");
        return nas_qos_cps_api_port_ingress_rollback(context, param, ix);

    case BASE_QOS_PORT_EGRESS_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PORT_EGRESS_OBJ");
        return nas_qos_cps_api_port_egress_rollback(context, param, ix);

    case BASE_QOS_BUFFER_POOL_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_POOL_OBJ");
        return nas_qos_cps_api_buffer_pool_rollback(context, param, ix);

    case BASE_QOS_BUFFER_PROFILE_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_PROFILE_OBJ");
        return nas_qos_cps_api_buffer_profile_rollback(context, param, ix);

    case BASE_QOS_PRIORITY_GROUP_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PRIORITY_GROUP_OBJ");
        return nas_qos_cps_api_priority_group_rollback(context, param, ix);

    case BASE_QOS_BUFFER_POOL_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_BUFFER_POOL_STAT_OBJ");
        return NAS_QOS_E_UNSUPPORTED;

    case BASE_QOS_PRIORITY_GROUP_STAT_OBJ:
        EV_LOG_TRACE(ev_log_t_QOS, 2, "NAS-QOS", "BASE_QOS_PRIORITY_GROUP_STAT_OBJ");
        return NAS_QOS_E_UNSUPPORTED;

    default:
        return NAS_QOS_E_UNSUPPORTED;

    }
}


