
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

static uint64_t TEST_SCHEDULER_ID_1;
static uint64_t TEST_SCHEDULER_ID_2;


using namespace std;

bool nas_qos_scheduler_create_test(uint64_t &sched_id) {

    printf("starting nas_qos_scheduler_create_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_ALGORITHM, BASE_QOS_SCHEDULING_TYPE_WDRR);
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_WEIGHT, 20);
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_METER_TYPE, 1);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MIN_RATE, 100);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MIN_BURST, 100);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MAX_RATE, 500);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MAX_BURST, 100);

    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned scheduler id for future retrieval
    cps_api_object_t recvd_obj = cps_api_object_list_get(tran.prev,0);
    cps_api_object_attr_t scheduler_attr = cps_api_get_key_data(recvd_obj, BASE_QOS_SCHEDULER_PROFILE_ID);

    if (scheduler_attr == NULL) {
        cout << "Key scheduler id is not returned\n";
        return false;
    }

    sched_id = cps_api_object_attr_data_u64(scheduler_attr);

    cout << " NAS returns scheduler Id: "<< sched_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_scheduler_get_test(uint64_t sched_id) {
    printf("starting nas_qos_scheduler_get_test, sched_id: %ld (0 for ALL)\n", sched_id);

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK)
        return false;


    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    if (sched_id)
        cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &sched_id, sizeof(uint64_t));

    bool rc = false;

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);

        for (; ix < mx; ++ix) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list, ix);
            cps_api_object_attr_t sched_id_attr = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID);
            if (sched_id_attr)
                cout << "scheduler id:  " << cps_api_object_attr_data_u64(sched_id_attr) << endl;
            cps_api_object_attr_t algorithm = cps_api_object_attr_get(obj, BASE_QOS_SCHEDULER_PROFILE_ALGORITHM);
            cps_api_object_attr_t weight = cps_api_object_attr_get(obj, BASE_QOS_SCHEDULER_PROFILE_WEIGHT);
            cps_api_object_attr_t meter_type = cps_api_object_attr_get(obj, BASE_QOS_SCHEDULER_PROFILE_METER_TYPE);
            cps_api_object_attr_t min_rate = cps_api_object_attr_get(obj, BASE_QOS_SCHEDULER_PROFILE_MIN_RATE);
            cps_api_object_attr_t min_burst = cps_api_object_attr_get(obj, BASE_QOS_SCHEDULER_PROFILE_MIN_BURST);
            cps_api_object_attr_t max_rate = cps_api_object_attr_get(obj, BASE_QOS_SCHEDULER_PROFILE_MAX_RATE);
            cps_api_object_attr_t max_burst = cps_api_object_attr_get(obj, BASE_QOS_SCHEDULER_PROFILE_MAX_BURST);

            if (algorithm)
                cout << "scheduling algorithm: "<< cps_api_object_attr_data_u32(algorithm) <<endl;

            if (weight)
                cout << "weight: "<< cps_api_object_attr_data_u32(weight) << endl;

            if (meter_type)
                cout << "shaper type: "<< cps_api_object_attr_data_u32(meter_type) << endl;

            if (min_rate)
                cout << "min rate: "<< cps_api_object_attr_data_u64(min_rate) << endl;

            if (min_burst)
                cout << "min burst: "<< cps_api_object_attr_data_u64(min_burst) << endl;

            if (max_rate)
                cout << "max_rate: "<< cps_api_object_attr_data_u64(max_rate) << endl;

            if (max_burst)
                cout << "max burst: "<< cps_api_object_attr_data_u64(max_burst) << endl;

        }

        rc = true;
    }

    cps_api_get_request_close(&gp);
    return rc;
}

bool nas_qos_scheduler_delete_test(uint64_t sched_id) {


    printf("starting nas_qos_scheduler_delete_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &sched_id, sizeof(uint64_t));

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

bool nas_qos_scheduler_set_test(uint64_t sched_id) {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_scheduler_set_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &sched_id, sizeof(uint64_t));

    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MAX_RATE, 800);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MIN_RATE, 200);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}


bool nas_qos_scheduler_set_npu_test() {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_scheduler_set_npu_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &TEST_SCHEDULER_ID_1, sizeof(uint64_t));

    // change to set on only npu 1 and 3
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_NPU_ID_LIST, 1);
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_NPU_ID_LIST, 3);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

bool nas_qos_scheduler_set_npu_and_attr_test() {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_scheduler_set_npu_and_attr_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_PROFILE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_PROFILE_ID,
            cps_api_object_ATTR_T_U64,
            &TEST_SCHEDULER_ID_1, sizeof(uint64_t));

    // change to set on only npu 2 and 3
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_NPU_ID_LIST, 2);
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_NPU_ID_LIST, 3);
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_MAX_RATE, 700);


    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

TEST(cps_api_events, init) {
    // basic test
    ASSERT_TRUE(nas_qos_scheduler_create_test(TEST_SCHEDULER_ID_1));
    ASSERT_TRUE(nas_qos_scheduler_create_test(TEST_SCHEDULER_ID_2));
    ASSERT_TRUE(nas_qos_scheduler_get_test(0)); //get ALL
    ASSERT_TRUE(nas_qos_scheduler_get_test(TEST_SCHEDULER_ID_1));
    ASSERT_TRUE(nas_qos_scheduler_delete_test(TEST_SCHEDULER_ID_2));
    ASSERT_TRUE(nas_qos_scheduler_delete_test(TEST_SCHEDULER_ID_1));

    // Set test
    ASSERT_TRUE(nas_qos_scheduler_create_test(TEST_SCHEDULER_ID_1));
    ASSERT_TRUE(nas_qos_scheduler_set_test(TEST_SCHEDULER_ID_1));
    ASSERT_TRUE(nas_qos_scheduler_get_test(TEST_SCHEDULER_ID_1));
    ASSERT_TRUE(nas_qos_scheduler_delete_test(TEST_SCHEDULER_ID_1));

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
 *  * nas_qos_scheduler_set_npu_and_attr_test requires proper npu ids to exist.
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
