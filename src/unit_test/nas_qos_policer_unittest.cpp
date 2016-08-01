
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


static uint64_t TEST_METER_ID_1;
static uint64_t TEST_METER_ID_2;


using namespace std;

bool nas_qos_policer_create_test(uint64_t &new_meter_id) {

    printf("starting nas_qos_policer_create_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_TYPE, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_MODE, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_COLOR_SOURCE, 2);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_YELLOW_PACKET_ACTION, 2);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_RED_PACKET_ACTION, 2);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_COMMITTED_BURST, 100);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_COMMITTED_RATE, 100);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_BURST, 250);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_RATE, 200);

    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned policer id for future retrieval
    cps_api_object_t recvd_obj = cps_api_object_list_get(tran.prev,0);
    cps_api_object_attr_t policer_attr = cps_api_get_key_data(recvd_obj, BASE_QOS_METER_ID);

    if (policer_attr == NULL) {
        cout << "Key switch id and policer id are not returned\n";
        return false;
    }

    new_meter_id = cps_api_object_attr_data_u64(policer_attr);

    cout << " NAS returns policer Id: "<< new_meter_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_policer_get_test(uint64_t meter_id) {
    printf("starting nas_qos_policer_get_test: %ld (0-all)\n", meter_id);

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK)
        return false;


    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    if (meter_id != 0) {
        cps_api_set_key_data(obj, BASE_QOS_METER_ID,
                cps_api_object_ATTR_T_U64,
                &meter_id, sizeof(uint64_t));
    }

    bool rc = false;

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {

        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);

        for (; ix < mx; ++ix) {
            obj = cps_api_object_list_get(gp.list, ix);
            cps_api_object_attr_t policer_type = cps_api_object_attr_get(obj, BASE_QOS_METER_TYPE);
            cps_api_object_attr_t peak_rate = cps_api_object_attr_get(obj, BASE_QOS_METER_PEAK_RATE);
            cps_api_object_attr_t peak_burst = cps_api_object_attr_get(obj, BASE_QOS_METER_PEAK_BURST);

            if (policer_type)
                cout << "policer type: "<< cps_api_object_attr_data_u32(policer_type) <<endl;

            if (peak_rate)
                cout << "peak rate: "<< cps_api_object_attr_data_u64(peak_rate) << endl;

            if (peak_burst)
                cout << "peak burst: "<< cps_api_object_attr_data_u64(peak_burst) << endl;
        }

        rc = true;
    }

    cps_api_get_request_close(&gp);
    return rc;
}

bool nas_qos_policer_delete_test(uint64_t meter_id) {


    printf("starting nas_qos_policer_delete_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &meter_id, sizeof(uint64_t));

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

bool nas_qos_policer_rollback_create_test() {

    printf("starting nas_qos_policer_rollback_create_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_TYPE, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_MODE, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_COLOR_SOURCE, 2);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_YELLOW_PACKET_ACTION, 2);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_RED_PACKET_ACTION, 2);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_COMMITTED_BURST, 100);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_COMMITTED_RATE, 100);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_BURST, 200);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_RATE, 200);

    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    // create a rollback scenario
    cps_api_object_t obj2 = cps_api_object_create();
    if (obj2 == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj2),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    // Intentionally not giving METER_TYPE so that the second creation will fail
    // cps_api_object_attr_add_u32(obj2, BASE_QOS_METER_TYPE, 1);
    cps_api_object_attr_add_u32(obj2, BASE_QOS_METER_MODE, 1);
    cps_api_object_attr_add_u32(obj2, BASE_QOS_METER_COLOR_SOURCE, 2);
    cps_api_object_attr_add_u32(obj2, BASE_QOS_METER_YELLOW_PACKET_ACTION, 2);
    cps_api_object_attr_add_u32(obj2, BASE_QOS_METER_RED_PACKET_ACTION, 2);
    cps_api_object_attr_add_u64(obj2, BASE_QOS_METER_COMMITTED_BURST, 100);
    cps_api_object_attr_add_u64(obj2, BASE_QOS_METER_COMMITTED_RATE, 100);
    cps_api_object_attr_add_u64(obj2, BASE_QOS_METER_PEAK_BURST, 200);
    cps_api_object_attr_add_u64(obj2, BASE_QOS_METER_PEAK_RATE, 200);

    if (cps_api_create(&tran, obj2) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed - Expected in Rollback test." <<endl;
    }

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_policer_rollback_delete_test() {


    printf("starting nas_qos_policer_rollback_delete_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &TEST_METER_ID_1, sizeof(uint64_t));

    if (cps_api_delete(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API DELETE FAILED" <<endl;
        return false;
    }

    cps_api_object_t obj2 = cps_api_object_create();
    if (obj2 == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj2),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    uint32_t TEST_METER_ID_6= 6;
    cps_api_set_key_data(obj2, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &TEST_METER_ID_6, sizeof(uint64_t));

    // bundle another delete to introduce a failure
    if (cps_api_delete(&tran, obj2) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed - Expected in Rollback test." <<endl;
    }

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_policer_set_test(uint64_t meter_id) {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_policer_set_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;


    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &meter_id, sizeof(uint64_t));

    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_RATE, 300);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_BURST, 350);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}


bool nas_qos_policer_set_npu_test() {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_policer_set_npu_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;


    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &TEST_METER_ID_1, sizeof(uint64_t));

    // change to set on only npu 1 and 3
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_NPU_ID_LIST, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_NPU_ID_LIST, 3);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

bool nas_qos_policer_set_npu_and_attr_test() {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_policer_set_npu_and_attr_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;


    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &TEST_METER_ID_1, sizeof(uint64_t));

    // change to set on only npu 2 and 3
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_NPU_ID_LIST, 2);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_NPU_ID_LIST, 3);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_RATE, 300);
    cps_api_object_attr_add_u64(obj, BASE_QOS_METER_PEAK_BURST, 350);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

TEST(cps_api_events, init) {
    // basic test
    ASSERT_TRUE(nas_qos_policer_create_test(TEST_METER_ID_1));
    ASSERT_TRUE(nas_qos_policer_create_test(TEST_METER_ID_2));
    ASSERT_TRUE(nas_qos_policer_get_test(0));  //get-all
    ASSERT_TRUE(nas_qos_policer_get_test(TEST_METER_ID_1));  //get-specific
    ASSERT_TRUE(nas_qos_policer_delete_test(TEST_METER_ID_1));
    ASSERT_TRUE(nas_qos_policer_delete_test(TEST_METER_ID_2));

    // rollback create test
    ASSERT_TRUE(nas_qos_policer_rollback_create_test());

    // Set test
    ASSERT_TRUE(nas_qos_policer_create_test(TEST_METER_ID_1));
    ASSERT_TRUE(nas_qos_policer_set_test(TEST_METER_ID_1));
    ASSERT_TRUE(nas_qos_policer_get_test(TEST_METER_ID_1)); // get specific
    ASSERT_TRUE(nas_qos_policer_delete_test(TEST_METER_ID_1));

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
 *  * nas_qos_policer_set_npu_and_attr_test requires proper npu ids to exist.
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
