
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
#include "cps_api_object_key.h"
#include "cps_class_map.h"

#include "gtest/gtest.h"

#include "dell-base-qos.h"


static uint64_t TEST_WRED_ID_1;
static uint64_t TEST_WRED_ID_2;

using namespace std;

bool nas_qos_wred_create_test(uint64_t &wred_id) {

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
        cout << "Key switch id and wred id are not returned\n";
        return false;
    }

    wred_id = cps_api_object_attr_data_u64(wred_attr);

    cout << " NAS returns wred Id: "<< wred_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_wred_get_test(uint64_t wred_id) {
    printf("starting nas_qos_wred_get_test: wred_id %ld (0 for ALL)\n", wred_id);

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK)
        return false;


    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_WRED_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    if (wred_id != 0)
        cps_api_set_key_data(obj, BASE_QOS_WRED_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &wred_id, sizeof(uint64_t));


    bool rc = false;

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);

        for (; ix < mx; ++ix) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list, ix);
            cps_api_object_attr_t g_enable = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_GREEN_ENABLE);
            cps_api_object_attr_t g_min = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_GREEN_MIN_THRESHOLD);
            cps_api_object_attr_t g_max = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_GREEN_MAX_THRESHOLD);
            cps_api_object_attr_t g_drop_prob = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_GREEN_DROP_PROBABILITY);
            cps_api_object_attr_t y_enable = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_YELLOW_ENABLE);
            cps_api_object_attr_t y_min = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_YELLOW_MIN_THRESHOLD);
            cps_api_object_attr_t y_max = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_YELLOW_MAX_THRESHOLD);
            cps_api_object_attr_t y_drop_prob = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_YELLOW_DROP_PROBABILITY);
            cps_api_object_attr_t r_enable = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_RED_ENABLE);
            cps_api_object_attr_t r_min = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_RED_MIN_THRESHOLD);
            cps_api_object_attr_t r_max = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_RED_MAX_THRESHOLD);
            cps_api_object_attr_t r_drop_prob = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_RED_DROP_PROBABILITY);
            cps_api_object_attr_t weight = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_WEIGHT);
            cps_api_object_attr_t ecn_enable = cps_api_object_attr_get(obj, BASE_QOS_WRED_PROFILE_ECN_ENABLE);


            if (g_enable)
                cout << "green enable: "<< cps_api_object_attr_data_u32(g_enable) <<endl;

            if (g_min)
                cout << "green min threshold: "<< cps_api_object_attr_data_u32(g_min) << endl;

            if (g_max)
                cout << "green max threshold: "<< cps_api_object_attr_data_u32(g_max) << endl;

            if (g_drop_prob)
                cout << "green drop probability: "<< cps_api_object_attr_data_u32(g_drop_prob) << endl;

            if (y_enable)
                cout << "yellow enable: "<< cps_api_object_attr_data_u32(y_enable) <<endl;

            if (y_min)
                cout << "yellow min threshold: "<< cps_api_object_attr_data_u32(y_min) << endl;

            if (y_max)
                cout << "yellow max threshold: "<< cps_api_object_attr_data_u32(y_max) << endl;

            if (y_drop_prob)
                cout << "yellow drop probability: "<< cps_api_object_attr_data_u32(y_drop_prob) << endl;

            if (r_enable)
                cout << "red enable: "<< cps_api_object_attr_data_u32(r_enable) <<endl;

            if (r_min)
                cout << "red min threshold: "<< cps_api_object_attr_data_u32(r_min) << endl;

            if (r_max)
                cout << "red max threshold: "<< cps_api_object_attr_data_u32(r_max) << endl;

            if (r_drop_prob)
                cout << "red drop probability: "<< cps_api_object_attr_data_u32(r_drop_prob) << endl;

            if (weight)
                cout << "weight: "<< cps_api_object_attr_data_u32(weight) << endl;

            if (ecn_enable)
                cout << "ecn_enable: "<< cps_api_object_attr_data_u32(ecn_enable) << endl;
        }

        rc = true;
    }

    cps_api_get_request_close(&gp);
    return rc;
}

bool nas_qos_wred_delete_test(uint64_t wred_id) {


    printf("starting nas_qos_wred_delete_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_WRED_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_WRED_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &wred_id, sizeof(uint64_t));

    if (cps_api_delete(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API DELETE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_wred_set_test(uint64_t wred_id) {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_wred_set_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_WRED_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_WRED_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &wred_id, sizeof(uint64_t));

    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_YELLOW_ENABLE, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_YELLOW_MIN_THRESHOLD, 50);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_YELLOW_MAX_THRESHOLD, 100);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}


bool nas_qos_wred_set_npu_test() {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_wred_set_npu_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_WRED_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_WRED_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &TEST_WRED_ID_1, sizeof(uint64_t));

    // change to set on only npu 1 and 3
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_NPU_ID_LIST, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_NPU_ID_LIST, 3);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

bool nas_qos_wred_set_npu_and_attr_test() {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_wred_set_npu_and_attr_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_WRED_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_WRED_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &TEST_WRED_ID_1, sizeof(uint64_t));

    // change to set on only npu 2 and 3
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_NPU_ID_LIST, 2);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_NPU_ID_LIST, 3);
    cps_api_object_attr_add_u32(obj, BASE_QOS_WRED_PROFILE_ECN_ENABLE, 0);


    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

TEST(cps_api_events, init) {
    // basic test
    ASSERT_TRUE(nas_qos_wred_create_test(TEST_WRED_ID_1));
    ASSERT_TRUE(nas_qos_wred_create_test(TEST_WRED_ID_2));
    ASSERT_TRUE(nas_qos_wred_set_test(TEST_WRED_ID_1));
    ASSERT_TRUE(nas_qos_wred_get_test(0)); //get all
    ASSERT_TRUE(nas_qos_wred_get_test(TEST_WRED_ID_1)); //get one
    ASSERT_TRUE(nas_qos_wred_delete_test(TEST_WRED_ID_1));
    ASSERT_TRUE(nas_qos_wred_delete_test(TEST_WRED_ID_2));

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
 *  * nas_qos_wred_set_npu_and_attr_test requires proper npu ids to exist.
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
