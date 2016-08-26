
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
 * \file   nas_qos_cps_priority_group.h
 * \brief  NAS qos priority_group CPS API routines
 * \date   03-2016
 * \author
 */
#ifndef _NAS_QOS_CPS_PRIORITY_GROUP_H_
#define _NAS_QOS_CPS_PRIORITY_GROUP_H_

#include "cps_api_operation.h"



/**
  * This function provides NAS-QoS priority group CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_priority_group_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix);

/**
  * This function provides NAS-QoS priority_group CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_priority_group_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix);
/**
  * This function provides NAS-QoS priority_group CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_priority_group_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix);

t_std_error nas_qos_port_priority_group_init(hal_ifindex_t ifindex);

cps_api_return_code_t nas_qos_cps_api_priority_group_stat_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix);
cps_api_return_code_t nas_qos_cps_api_priority_group_stat_clear (void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix);

#endif
