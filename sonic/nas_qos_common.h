
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
 * \file   nas_qos_common.h
 * \brief  NAS QOS Common macros and typedefs
 */

#ifndef _NAS_QOS_COMMON_H_
#define _NAS_QOS_COMMON_H_

#include "nas_types.h"
#include "dell-base-qos.h"
#include "ietf-inet-types.h"
#include "nas_ndi_qos.h"

/** NAS QOS Error Codes */
#define NAS_QOS_E_NONE            (int)STD_ERR_OK
#define    NAS_QOS_E_MEM            (int)STD_ERR (QOS, NONMEM, 0)

#define NAS_QOS_E_MISSING_KEY   (int)STD_ERR (QOS, CFG, 1)
#define NAS_QOS_E_MISSING_ATTR  (int)STD_ERR (QOS, CFG, 2)
#define NAS_QOS_E_UNSUPPORTED   (int)STD_ERR (QOS, CFG, 3) // Unsupported attribute
#define NAS_QOS_E_DUPLICATE     (int)STD_ERR (QOS, CFG, 4) // Attribute duplicated in CPS object
#define NAS_QOS_E_ATTR_LEN      (int)STD_ERR (QOS, CFG, 5) // Unexpected attribute length

#define NAS_QOS_E_CREATE_ONLY   (int)STD_ERR (QOS, PARAM, 1) // Modify attempt on create-only attribute
#define NAS_QOS_E_ATTR_VAL      (int)STD_ERR (QOS, PARAM, 2) // Wrong value for attribute
#define NAS_QOS_E_INCONSISTENT  (int)STD_ERR (QOS, PARAM, 3) // Attribute Value inconsistent
                                                             // with other attributes

#define NAS_QOS_E_KEY_VAL       (int)STD_ERR (QOS, NEXIST, 0) // No object with this key

#define NAS_QOS_E_FAIL          (int)STD_ERR (QOS, FAIL, 0) // All other run time failures



// TYPE MASK prepended to the nas-obj-id to make QUEUE-id and Scheduler-id
// unique across the NAS subsystem
#define NAS_QUEUE_ID_TYPE_MASK              0x0001000000000000UL
#define NAS_SCHEDULER_GROUP_ID_TYPE_MASK    0x0002000000000000UL

typedef ndi_qos_map_type_t nas_qos_map_type_t;

// Making complex keys
#define MAKE_KEY(key1, key2)     (((key2) << 16) | (key1))
#define GET_KEY1(key)            (((key).any) & 0xFFFF)
#define GET_KEY2(key)            ((((key).any) >> 16) & 0xFFFF)


typedef union nas_qos_map_entry_key_t {
    /* The following field are selectively filled
     * based on the type of qos map.
     * TC, DSCP, DOT1P, pfc-priority are all casted into uint_t
     * so the map_entry can have uniformed uint_t key.
     */
    // Traffic Class
    uint_t    tc;

    // DSCP value
    uint_t  dscp;

    // dot1p value
    uint_t  dot1p;

    // tc-queue-type combination key
    uint_t  tc_queue_type;

    // tc-color-type combination key
    uint_t  tc_color;

    // PG-PFC combination key: PG-to-PFC-priority map entry key
    uint_t    pg_prio;

    // PFC-queue-type combination key: PFC-priority-to-queue map entry key
    uint_t    prio_queue_type;

    // generic reference to any of the above fields
    uint_t  any;

} nas_qos_map_entry_key_t;

typedef struct nas_qos_map_entry_value_t {
    /* The following field are selectively filled
     * based on the type of qos map.
     */
    // TC and Color are used by dot1p|dscp to TC|Color map
    // Traffic Class
    BASE_CMN_TRAFFIC_CLASS_t tc;

    // Color
    BASE_QOS_PACKET_COLOR_t color;

    // DSCP and dot1p are used by TC to dscp|dot1p map
    // DSCP value
    INET_DSCP_t   dscp;

    // dot1p value
    BASE_CMN_DOT1P_t  dot1p;

    // Queue number
    uint_t     queue_num;

    // local priority group id
    uint_t         pg;

} nas_qos_map_entry_value_t;



#endif
