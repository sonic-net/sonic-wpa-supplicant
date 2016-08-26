
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
 * \file   nas_qos_cps_buffer_pool.h
 * \brief  NAS qos buffer_pool CPS API routines
 */
#ifndef _NAS_QOS_CPS_BUFFER_POOL_H_
#define _NAS_QOS_CPS_BUFFER_POOL_H_

#include "cps_api_operation.h"



/**
  * This function provides NAS-QoS buffer pool CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_pool_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix);

/**
  * This function provides NAS-QoS buffer_pool CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_pool_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix);
/**
  * This function provides NAS-QoS buffer_pool CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_buffer_pool_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix);

cps_api_return_code_t nas_qos_cps_api_buffer_pool_stat_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix);
#endif
