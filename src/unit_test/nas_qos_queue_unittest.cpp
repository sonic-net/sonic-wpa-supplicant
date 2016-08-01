
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


#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <assert.h>

#include "cps_api_events.h"
#include "cps_api_key.h"
#include "cps_api_operation.h"
#include "cps_api_object.h"
#include "cps_api_errors.h"
#include "cps_class_map.h"
#include "cps_api_object_key.h"

#include "gtest/gtest.h"

#include "dell-base-qos.h"
#include "nas_qos_ifindex_utl.h"

using namespace std;

static uint32_t TEST_PORT_ID; // ifindex for npu 0 port 1 (1/1)in S6k
static uint32_t CPU_PORT_ID; // ifindex for cpu port "npu-0" in S6k
static uint64_t TEST_WRED_ID;

#define ANYCAST 1
#define UCAST   2
#define MCAST   3

bool nas_qos_queue_get_test(uint32_t ifindex, uint32_t type_key, uint32_t local_q_id_key,
                            bool type_specified, bool local_q_id_specified) {
    printf("starting nas_qos_queue_get_test\n");

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_QUEUE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_QUEUE_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &ifindex, sizeof(uint32_t));

    if (type_specified)
        cps_api_set_key_data(obj, BASE_QOS_QUEUE_TYPE,
            cps_api_object_ATTR_T_U32,
            &type_key, sizeof(uint32_t));
    if (local_q_id_specified)
        cps_api_set_key_data(obj, BASE_QOS_QUEUE_QUEUE_NUMBER,
            cps_api_object_ATTR_T_U32,
            &local_q_id_key, sizeof(uint32_t));
    bool rc = false;

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);

        for (; ix < mx; ++ix) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list, ix);
            cps_api_object_attr_t port_id = cps_api_get_key_data(obj, BASE_QOS_QUEUE_PORT_ID);
            cps_api_object_attr_t type = cps_api_get_key_data(obj, BASE_QOS_QUEUE_TYPE);
            cps_api_object_attr_t local_q_id = cps_api_get_key_data(obj, BASE_QOS_QUEUE_QUEUE_NUMBER);

            cps_api_object_attr_t queue_id = cps_api_object_attr_get(obj, BASE_QOS_QUEUE_ID);
            cps_api_object_attr_t wred_id = cps_api_object_attr_get(obj, BASE_QOS_QUEUE_WRED_ID);
            cps_api_object_attr_t scheduler_id = cps_api_object_attr_get(obj, BASE_QOS_QUEUE_SCHEDULER_PROFILE_ID );

            if (port_id)
                cout << " port_id: "<< cps_api_object_attr_data_u32(port_id) ;

            if (type)
                cout << " type: "<< cps_api_object_attr_data_u32(type);

            if (local_q_id)
                cout << " local_q_id: "<< cps_api_object_attr_data_u32(local_q_id);

            if (queue_id) {
                uint64_t val64 = cps_api_object_attr_data_u64(queue_id);
                printf(" Queue Id: 0x%016lX\n", val64);
            }

            if (wred_id)
                cout << " wred id: "<< cps_api_object_attr_data_u64(wred_id);

            if (scheduler_id)
                cout << " scheduler profile: "<< cps_api_object_attr_data_u64(scheduler_id);

            if (wred_id || scheduler_id)
                printf("\n");
        }

        rc = true;
    }

    cps_api_get_request_close(&gp);

    return rc;
}


bool nas_qos_queue_set_wred_test(uint32_t ifindex, uint32_t type, uint32_t local_q_id) {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_queue_set_wred_test: ifindex %u, type %u, local_q_id %u\n",
            ifindex, type, local_q_id);

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_QUEUE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_QUEUE_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &ifindex, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_TYPE,
            cps_api_object_ATTR_T_U32,
            &type, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_QUEUE_NUMBER,
            cps_api_object_ATTR_T_U32,
            &local_q_id, sizeof(uint32_t));

    cps_api_object_attr_add_u64(obj, BASE_QOS_QUEUE_WRED_ID, TEST_WRED_ID);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

bool nas_qos_wred_create_test() {

    printf("starting nas_qos_wred_create_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_key_t key;
    cps_api_key_init(&key,
                     cps_api_qualifier_TARGET,
                     (cps_api_object_category_types_t)cps_api_obj_CAT_BASE_QOS,
                     BASE_QOS_WRED_PROFILE_OBJ,
                     0);

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_WRED_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_GREEN_ENABLE, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_GREEN_MIN_THRESHOLD, 20);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_GREEN_MAX_THRESHOLD, 80);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_GREEN_DROP_PROBABILITY, 10);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_YELLOW_ENABLE, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_YELLOW_MIN_THRESHOLD, 40);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_YELLOW_MAX_THRESHOLD, 60);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_YELLOW_DROP_PROBABILITY, 30);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_WEIGHT, 8);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_ECN_ENABLE, 1);

    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned wred id for future retrieval
    cps_api_object_t recvd_obj = cps_api_object_list_get(tran.prev,0);
    cps_api_object_attr_t wred_attr = cps_api_get_key_data(recvd_obj, BASE_QOS_WRED_PROFILE_ID);

    if (wred_attr == NULL) {
        cout << "Key wred id not returned\n";
        return false;
    }

    TEST_WRED_ID = cps_api_object_attr_data_u64(wred_attr);

    cout <<  " NAS returns wred Id: "<< TEST_WRED_ID  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_queue_get_stat_test(uint32_t ifindex, uint32_t type, uint32_t local_q_id)
{
    printf("starting nas_qos_queue_get_stat_test: ifindex %u type %u q_number %u\n",
                ifindex, type, local_q_id);

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_QUEUE_STAT_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_QUEUE_STAT_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &ifindex, sizeof(uint32_t));

    cps_api_set_key_data(obj, BASE_QOS_QUEUE_STAT_TYPE,
            cps_api_object_ATTR_T_U32,
            &type, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_STAT_QUEUE_NUMBER,
            cps_api_object_ATTR_T_U32,
            &local_q_id, sizeof(uint32_t));

    // fill some attributes to get
    cps_api_object_attr_add_u64(obj, BASE_QOS_QUEUE_STAT_PACKETS, 0);
    cps_api_object_attr_add_u64(obj, BASE_QOS_QUEUE_STAT_BYTES, 0);

    bool rc = false;

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);

        for (; ix < mx; ++ix) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list, ix);

            cps_api_object_attr_t packets = cps_api_object_attr_get(obj, BASE_QOS_QUEUE_STAT_PACKETS);
            cps_api_object_attr_t bytes = cps_api_object_attr_get(obj, BASE_QOS_QUEUE_STAT_BYTES);
            cps_api_object_attr_t green_dropped_packets = cps_api_object_attr_get(obj, BASE_QOS_QUEUE_STAT_GREEN_DROPPED_PACKETS);
            cps_api_object_attr_t green_dropped_bytes = cps_api_object_attr_get(obj,  BASE_QOS_QUEUE_STAT_GREEN_DROPPED_BYTES);

            if (packets)
                cout << " packets: " << cps_api_object_attr_data_u64(packets) << endl;

            if (bytes)
                cout << " bytes: " << cps_api_object_attr_data_u64(bytes) << endl;

            if (green_dropped_packets)
                cout << " green_dropped_packets: " << cps_api_object_attr_data_u64(green_dropped_packets) <<endl;

            if (green_dropped_bytes)
                cout << " green_dropped_bytes: " << cps_api_object_attr_data_u64(green_dropped_bytes) << endl;

        }

        rc = true;
    }

    cps_api_get_request_close(&gp);

    return rc;
}

bool nas_qos_queue_clear_stat_test(uint32_t ifindex, uint32_t type, uint32_t local_q_id)
{
    printf("starting nas_qos_queue_clear_stat_test: ifindex %u type %u q_number %u\n",
                ifindex, type, local_q_id);

    cps_api_transaction_params_t trans;

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_QUEUE_STAT_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_QUEUE_STAT_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &ifindex, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_STAT_TYPE,
            cps_api_object_ATTR_T_U32,
            &type, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_STAT_QUEUE_NUMBER,
            cps_api_object_ATTR_T_U32,
            &local_q_id, sizeof(uint32_t));

    // fill some counters to clear
    cps_api_object_attr_add_u64(obj, BASE_QOS_QUEUE_STAT_PACKETS, 0);
    cps_api_object_attr_add_u64(obj, BASE_QOS_QUEUE_STAT_BYTES, 0);


    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;

}



TEST(cps_api_events, init) {
    // basic test
    char if_name[32];
    if (!nas_qos_get_first_phy_port(if_name, sizeof(if_name), TEST_PORT_ID)) {
        return;
    }
    printf("front_port %s ifindex %u\n", if_name, TEST_PORT_ID);
    if (!nas_qos_get_if_index("npu-0", CPU_PORT_ID)) {
        return;
    }

    ASSERT_TRUE(nas_qos_queue_get_test(TEST_PORT_ID, UCAST, 1, true, true));  //get one
    ASSERT_TRUE(nas_qos_queue_get_test(TEST_PORT_ID, 0, 0, false, false));  //get all

    // Set test one particular queue
    ASSERT_TRUE(nas_qos_wred_create_test());
    ASSERT_TRUE(nas_qos_queue_set_wred_test(TEST_PORT_ID, UCAST, 5));

    // get individual queue
    ASSERT_TRUE(nas_qos_queue_get_test(TEST_PORT_ID, UCAST, 5, true, true));
    ASSERT_TRUE(nas_qos_queue_get_test(TEST_PORT_ID, UCAST, 8, true, true)); // ucast q: 0..7
    ASSERT_TRUE(nas_qos_queue_get_test(TEST_PORT_ID, MCAST, 4, true, true)); // mcast q: 0..3

    ASSERT_TRUE(nas_qos_queue_get_test(TEST_PORT_ID, 0, 0, false, false));


    ASSERT_TRUE(nas_qos_queue_get_test(CPU_PORT_ID, 0, 0, false, false)); // get all

    // get queue stats
    ASSERT_TRUE(nas_qos_queue_get_stat_test(TEST_PORT_ID, UCAST, 2));
    ASSERT_TRUE(nas_qos_queue_clear_stat_test(TEST_PORT_ID, UCAST, 2));

}


/****** README
 *
 * This test expects a proper switch config at: /etc/opt/dell/os10/switch.xml
 * at the switch boot up time so that NAS-QOS can load the switch
 * configuration before running this set of unit test cases.
 *
 * (currently DN_SWITCH_CFG is set to /etc/opt/dell/os10/switch.xml)
 *
 *  * All test cases require a proper switch ID exists in the config.
 *
 ********** Start of a sample /etc/opt/dell/os10/switch.xml *****

<?xml version="1.0" encoding="UTF-8"?>
<switch_config>
<switch_topology switch_ids="0,1" />
<switch id="0" npus="0,1,2,3,4" />
<switch id="1" npus="5,6" />
</switch_config>

***************** End of the file ********************/


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
