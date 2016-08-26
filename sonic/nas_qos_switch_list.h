
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
 * \file   nas_qos_switch_list.h
 * \brief  API to cache the list of switches in the system
 */

#ifndef _NAS_QAS_SWITCH_LIST_H_
#define _NAS_QAS_SWITCH_LIST_H_

#include "std_type_defs.h"
#include "nas_qos_switch.h"
#include "hal_if_mapping.h" //interface_ctrl_t

/**
 *  This function gets a switch instance from the cached switch list in QoS
 *  @Param switch_id
 *  @Return QoS Switch Object pointer; NULL if not found
 */
nas_qos_switch* nas_qos_get_switch (uint_t switch_id);

/**
 * This function adds a switch instance to QoS switch list
 * @Param switch_id
 * @Param QoS switch instance
 * @return standard error code
 */
t_std_error    nas_qos_add_switch (uint_t switch_id, nas_qos_switch& switch_instance);

/**
 * This function removes a switch instance from QoS switch list
 * @Param switch_id
 * @return standard error code
 */
t_std_error nas_qos_remove_switch (uint32_t switch_id);

/**
 *  This function returns the switch pointer that contains the specified npu_id
 *  @Param npu id
 *  @Return Switch instance pointer if found
 */
nas_qos_switch * nas_qos_get_switch_by_npu(npu_id_t npu_id);

/*
 * This function fills in the interface_ctrl_t structure given an ifindex
 * @Param ifindex
 * @Return True if the interface structure is properly filled; False otherwise
 */
bool nas_qos_get_port_intf(uint_t ifindex, interface_ctrl_t *intf_ctrl);

#endif
