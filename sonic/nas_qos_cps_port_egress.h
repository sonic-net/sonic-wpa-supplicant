
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
 * \file   nas_qos_cps_port_egress.h
 * \brief  NAS qos port egress CPS API routines
 */
#ifndef _NAS_QOS_CPS_PORT_EGRESS_H_
#define _NAS_QOS_CPS_PORT_EGRESS_H_

#include "cps_api_operation.h"



/**
  * This function provides NAS-QoS PORT EGRESS CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_egress_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix);

/**
  * This function provides NAS-QoS PORT EGRESS CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_egress_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix);
/**
  * This function provides NAS-QoS PORT EGRESS CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_port_egress_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix);

#endif
