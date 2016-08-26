
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

static uint64_t test_map_id;
static uint64_t test_map_id_2;

using namespace std;


bool nas_qos_dot1p_tc_color_map_create_test(uint64_t &map_id) {

    printf("starting nas_qos_dot1p_tc_color_map_create_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ,
            cps_api_qualifier_TARGET);

    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned dot1p_tc_color_map id for future retrieval
    cps_api_object_t recvd_obj = cps_api_object_list_get(tran.prev,0);
    cps_api_object_attr_t map_id_attr = cps_api_get_key_data(recvd_obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID);

    if (map_id_attr == NULL) {
        cout << "Key switch id and dot1p_tc_color_map id are not returned\n";
        return false;
    }

    map_id = cps_api_object_attr_data_u64(map_id_attr);

    cout << " NAS returns dot1p_tc_color_map Id: "<< map_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_dot1p_tc_color_map_get_test(uint64_t map_id) {
    printf("starting nas_qos_dot1p_tc_color_map_get_test, map_id: %ld (0 for ALL)\n", map_id);

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK)
        return false;


    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ,
            cps_api_qualifier_TARGET);

    if (map_id)
        cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    bool rc = false;

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);

        for (; ix < mx; ++ix) {
            obj = cps_api_object_list_get(gp.list, ix);
            // Map id exists inside the key, no other info at this level
            cps_api_object_attr_t map_id_attr = cps_api_get_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID);
            if (map_id_attr)
                cout << "dot1p to tc color map " << cps_api_object_attr_data_u64(map_id_attr) << " is created" << endl;
        }
        rc = true;
    }
    else {
        cout << "dot1p to tc color map " << map_id << " is not created" << endl;
    }

    cps_api_get_request_close(&gp);
    return rc;
}

bool nas_qos_dot1p_tc_color_map_entry_get_test(uint64_t map_id, uint8_t dot1p) {
    printf("starting nas_qos_dot1p_tc_color_map_get_test\n");

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK)
        return false;


    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_DOT1P,
            cps_api_object_ATTR_T_BIN,
            &dot1p, sizeof(uint8_t));

    bool rc = false;

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);

        for (; ix < mx; ++ix) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list, ix);
            cps_api_object_attr_t tc_attr = cps_api_object_attr_get(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_TC);
            cps_api_object_attr_t color_attr = cps_api_object_attr_get(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_COLOR);
            if (tc_attr)
                printf("dot1p: %u, tc: %u\n", dot1p, *(uint8_t*)cps_api_object_attr_data_bin(tc_attr));
            if (color_attr)
                printf("dot1p: %u, color: %u\n", dot1p, cps_api_object_attr_data_u32(color_attr));

        }

        rc = true;
    }

    cps_api_get_request_close(&gp);
    return rc;
}

bool nas_qos_dot1p_tc_color_map_delete_test(uint64_t map_id) {


    printf("starting nas_qos_dot1p_tc_color_map_delete_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

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

bool nas_qos_dot1p_tc_color_map_entry_create_test(uint64_t map_id, uint8_t dot1p,
        uint16_t tc, uint16_t color) {

    printf("starting nas_qos_dot1p_tc_color_map_entry_create_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_DOT1P,
            cps_api_object_ATTR_T_BIN,
            &dot1p, sizeof(uint8_t));

    cps_api_object_attr_add(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_TC, &tc, sizeof(uint8_t));
    cps_api_object_attr_add_u32(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_COLOR, color);


    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned dot1p_tc_color_map id for future retrieval
    printf ("creating dot1p_tc_color_map entry: dot1p %u\n", dot1p );

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}


bool nas_qos_dot1p_tc_color_map_set_test(uint64_t map_id, uint8_t dot1p,
        uint16_t tc, uint16_t color) {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_dot1p_tc_color_map_set_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));
    // Set is operable only on dot1p-tc mapping entries
    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_DOT1P,
            cps_api_object_ATTR_T_BIN,
            &dot1p, sizeof(uint8_t));

    cps_api_object_attr_add(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_TC, &tc, sizeof(uint8_t));
    cps_api_object_attr_add_u32(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_COLOR, color);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

bool nas_qos_dot1p_tc_color_map_entry_delete_test(uint64_t map_id, uint8_t dot1p) {

    printf("starting nas_qos_dot1p_tc_color_map_entry_delete_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_DOT1P,
            cps_api_object_ATTR_T_BIN,
            &dot1p, sizeof(uint8_t));


    if (cps_api_delete(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned dot1p_tc_color_map id for future retrieval
    printf ("deleting dot1p_tc_color_map entry: dot1p %u\n", dot1p );

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}



TEST(cps_api_events, init) {
    // basic test for map
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_create_test(test_map_id));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_create_test(test_map_id_2));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_create_test(test_map_id, 0, 1, 0));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_create_test(test_map_id_2, 0, 1, 0));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_get_test(0));  //get all
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_get_test(test_map_id)); //get one
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_delete_test(test_map_id, 0));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_delete_test(test_map_id_2, 0));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_delete_test(test_map_id));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_delete_test(test_map_id_2));

    // test for map entries create/modify
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_create_test(test_map_id));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_create_test(test_map_id, 0, 1, 0));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_get_test(test_map_id, 0));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_create_test(test_map_id, 4, 0, 0));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_get_test(test_map_id, 4));
    // set
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_set_test(test_map_id, 4, 4, 1));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_get_test(test_map_id, 4));

    // delete entries
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_delete_test(test_map_id, 4));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_entry_delete_test(test_map_id, 0));
    ASSERT_TRUE(nas_qos_dot1p_tc_color_map_delete_test(test_map_id));

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
