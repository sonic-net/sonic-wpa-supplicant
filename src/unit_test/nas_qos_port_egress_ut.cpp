
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
#include <vector>
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

// temporarily disable tc and color to dscp/dot1p map and waiting
// for support from SAI
#define DISABLE_TC_COLOR_MAP

#define MAX_TEST_PORT_ID     12
static uint32_t test_port_id;

uint64_t tc_queue_map_id;
#ifndef DISABLE_TC_COLOR_MAP
uint64_t tc_color_dot1p_map_id, tc_color_dscp_map_id;
#endif

bool nas_qos_port_egress_get_test(uint32_t nas_port_id) {
    cout << __FUNCTION__ << ": entering, port=" << nas_port_id << endl;

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK) {
        return false;
    }

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL) {
        return false;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
            BASE_QOS_PORT_EGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_EGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &nas_port_id, sizeof(uint32_t));

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t idx;
        size_t obj_num = cps_api_object_list_size(gp.list);
        cps_api_object_attr_t attr;
        cps_api_object_t obj;

        for (idx = 0; idx < obj_num; idx ++) {
            obj = cps_api_object_list_get(gp.list, idx);
            if (obj == NULL) {
                cout << "failed to get object " << idx << " from list" << endl;
                continue;
            }

            cps_api_object_it_t it;
            cps_api_object_it_begin(obj, &it);
            vector<uint64_t> queue_list;
            for ( ; cps_api_object_it_valid(&it); cps_api_object_it_next(&it)) {
                cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
                if (id == BASE_QOS_PORT_EGRESS_QUEUE_ID_LIST) {
                    queue_list.push_back(cps_api_object_attr_data_u64(it.attr));
                }
            }

            attr = cps_api_get_key_data(obj, BASE_QOS_PORT_EGRESS_PORT_ID);
            if (attr == NULL) {
                cout << "failed to get port id, obj_id=" << idx << endl;
                continue;
            }
            uint32_t port_id = cps_api_object_attr_data_u32(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_BUFFER_LIMIT);
            if (attr == NULL) {
                cout << "failed to get buffer limit, obj_id=" << idx << endl;
                continue;
            }
            uint64_t buffer_limit = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_WRED_PROFILE_ID);
            if (attr == NULL) {
                cout << "failed to get wred profile id, obj_id=" << idx << endl;
                continue;
            }
            uint64_t wred_profile_id = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_NUM_UNICAST_QUEUE);
            if (attr == NULL) {
                cout << "failed to get number of unicast queue, obj_id=" << idx << endl;
                continue;
            }
            uint8_t num_ucast_queue = ((uint8_t *)cps_api_object_attr_data_bin(attr))[0];

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_NUM_MULTICAST_QUEUE);
            if (attr == NULL) {
                cout << "failed to get number of multicast queue, obj_id=" << idx << endl;
                continue;
            }
            uint8_t num_mcast_queue = ((uint8_t *)cps_api_object_attr_data_bin(attr))[0];

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_NUM_QUEUE);
            if (attr == NULL) {
                cout << "failed to get number of queue, obj_id=" << idx << endl;
                continue;
            }
            uint8_t num_queue = ((uint8_t *)cps_api_object_attr_data_bin(attr))[0];

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP);
            if (attr == NULL) {
                cout << "failed to get tc to queue map, obj_id=" << idx << endl;
                continue;
            }
            uint64_t tc_to_queue = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_TC_TO_DOT1P_MAP);
            if (attr == NULL) {
                cout << "failed to get tc to dot1p map, obj_id=" << idx << endl;
                continue;
            }
            uint64_t tc_to_dot1p = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_TC_TO_DSCP_MAP);
            if (attr == NULL) {
                cout << "failed to get tc to dscp map, obj_id=" << idx << endl;
                continue;
            }
            uint64_t tc_to_dscp = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP);
            if (attr == NULL) {
                cout << "failed to get tc color to dot1p map, obj_id=" << idx << endl;
                continue;
            }
            uint64_t tc_color_to_dot1p = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP);
            if (attr == NULL) {
                cout << "failed to get tc color to dscp map, obj_id=" << idx << endl;
                continue;
            }
            uint64_t tc_color_to_dscp = cps_api_object_attr_data_u64(attr);


            cout << "Object " << idx << endl;
            cout << "  port                      : " << port_id << endl;
            cout << "  rate buffer limit         : " << buffer_limit << endl;
            cout << "  wred profile id           : " << wred_profile_id << endl;
            cout << "  num of unicast queue      : " << (int)num_ucast_queue << endl;
            cout << "  num of multicast queue    : " << (int)num_mcast_queue << endl;
            cout << "  num of queue              : " << (int)num_queue << endl;
            for (auto queue_id: queue_list) {
                cout << "  queue_id                  : " << queue_id << endl;
            }
            cout << "  tc to queue map           : " << tc_to_queue << endl;
            cout << "  tc to dot1p map           : " << tc_to_dot1p << endl;
            cout << "  tc to dscp map            : " << tc_to_dscp << endl;
            cout << "  tc and color to dot1p map : " << tc_color_to_dot1p << endl;
            cout << "  tc and color to dscp map  : " << tc_color_to_dscp << endl;
            cout << endl;
        }
    }

    return true;
}

static struct {
    const char *name;
    cps_api_attr_id_t obj_attr;
    cps_api_attr_id_t map_id_attr;
    uint64_t *data;
} map_info_list[] = {
#ifndef DISABLE_TC_COLOR_MAP
    {"tc_color_to_dot1p", BASE_QOS_TC_COLOR_TO_DOT1P_MAP_OBJ,
        BASE_QOS_TC_COLOR_TO_DOT1P_MAP_ID, &tc_color_dot1p_map_id},
    {"tc_color_to_dscp", BASE_QOS_TC_COLOR_TO_DSCP_MAP_OBJ,
        BASE_QOS_TC_COLOR_TO_DSCP_MAP_ID, &tc_color_dscp_map_id},
#endif
    {"tc_to_queue", BASE_QOS_TC_TO_QUEUE_MAP_OBJ,
        BASE_QOS_TC_TO_QUEUE_MAP_ID, &tc_queue_map_id},
    {NULL, 0, 0, NULL}
};

static bool nas_qos_map_create(cps_api_attr_id_t obj_attr_id,
                            cps_api_attr_id_t map_attr_id,
                            const char *map_name, uint64_t& map_id)
{
    cout << "Starting create " << map_name << " map" << endl;
    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL) {
        return false;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), obj_attr_id,
            cps_api_qualifier_TARGET);

    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned dot1p_color_map id for future retrieval
    cps_api_object_t recvd_obj = cps_api_object_list_get(tran.prev,0);
    cps_api_object_attr_t map_id_attr = cps_api_get_key_data(recvd_obj, map_attr_id);

    if (map_id_attr == NULL) {
        cout << "Key  dot1p_color_map id not returned\n";
        return false;
    }

    map_id = cps_api_object_attr_data_u64(map_id_attr);

    cout << " NAS returns " << map_name << " Id: "<< map_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_port_egress_map_create(void)
{
    for (int id = 0; map_info_list[id].name != NULL; id ++) {
        if (!nas_qos_map_create(map_info_list[id].obj_attr,
                        map_info_list[id].map_id_attr, map_info_list[id].name,
                        *map_info_list[id].data)) {
            cout << "Failed to create " << map_info_list[id].name << endl;
            return false;
        }
    }

    return true;
}

static bool nas_qos_map_delete(cps_api_attr_id_t obj_attr_id,
                            cps_api_attr_id_t map_attr_id,
                            const char *map_name, uint64_t map_id)
{

    cout << "Staring delete " << map_name << " map, id=" << map_id << endl;

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), obj_attr_id,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, map_attr_id,
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

static bool nas_qos_tc2queue_entry_create(uint64_t map_id, uint8_t tc, uint32_t type,
                                        uint8_t queue)
{
    cout << "Starting create tc_to_queue map entry" << endl;
    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK) {
        return false;
    }

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL) {
        return false;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_QOS_TC_TO_QUEUE_MAP_ENTRY,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ID,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TC,
            cps_api_object_ATTR_T_BIN,
            &tc, sizeof(uint8_t));

    //type: 0-none 1-unicast 2-multicast
    cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TYPE,
            cps_api_object_ATTR_T_U32,
            &type, sizeof(uint32_t));

    cps_api_object_attr_add_u32(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_QUEUE_NUMBER, queue);

    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned tc_queue_map id for future retrieval
    printf ("creating tc_queue_map entry: tc %u type %u\n", tc, type );

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_port_egress_map_entry_create(void)
{
    bool ret = false;

    do {
        if (!nas_qos_tc2queue_entry_create(tc_queue_map_id, 0, 1, 0)) break;
        if (!nas_qos_tc2queue_entry_create(tc_queue_map_id, 2, 1, 1)) break;
        if (!nas_qos_tc2queue_entry_create(tc_queue_map_id, 3, 1, 2)) break;
        if (!nas_qos_tc2queue_entry_create(tc_queue_map_id, 0, 2, 0)) break;
        if (!nas_qos_tc2queue_entry_create(tc_queue_map_id, 2, 2, 1)) break;
        if (!nas_qos_tc2queue_entry_create(tc_queue_map_id, 3, 2, 2)) break;
        ret = true;
    } while(0);

    return ret;
}

static bool nas_qos_tc2queue_entry_delete(uint64_t map_id, uint8_t tc, uint32_t type)
{
    cout << "Starting delete tc_to_queue map entry" << endl;

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_QOS_TC_TO_QUEUE_MAP_ENTRY,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ID,
            cps_api_object_ATTR_T_U64,
            &map_id, sizeof(uint64_t));

    cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TC,
            cps_api_object_ATTR_T_BIN,
            &tc, sizeof(uint8_t));

    cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TYPE,
            cps_api_object_ATTR_T_U32,
            &type, sizeof(uint32_t));


    if (cps_api_delete(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned tc_queue_map id for future retrieval
    printf ("deleting tc_queue_map entry: tc %u, type %u\n", tc, type );

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_port_egress_map_entry_delete(void)
{
    bool ret = false;

    do {
        if (!nas_qos_tc2queue_entry_delete(tc_queue_map_id, 0, 1)) break;
        if (!nas_qos_tc2queue_entry_delete(tc_queue_map_id, 2, 1)) break;
        if (!nas_qos_tc2queue_entry_delete(tc_queue_map_id, 3, 1)) break;
        if (!nas_qos_tc2queue_entry_delete(tc_queue_map_id, 0, 2)) break;
        if (!nas_qos_tc2queue_entry_delete(tc_queue_map_id, 2, 2)) break;
        if (!nas_qos_tc2queue_entry_delete(tc_queue_map_id, 3, 2)) break;
        ret = true;
    } while(0);

    return ret;
}

bool nas_qos_port_egress_set_test(uint32_t nas_port_id) {
    cps_api_transaction_params_t trans;

    cout << __FUNCTION__ << ": entering, port=" << nas_port_id << endl;

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) {
        return false;
    }

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL) {
        return false;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
            BASE_QOS_PORT_EGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_EGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &nas_port_id, sizeof(uint32_t));

    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP, tc_queue_map_id);
#ifndef DISABLE_TC_COLOR_MAP
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP, tc_color_dot1p_map_id);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP, tc_color_dscp_map_id);
#endif

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) {
        cout << "Failed to set cps object" << endl;
        return false;
    }

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) {
        cout << "Failed to commit" << endl;
        return false;
    }

    cps_api_transaction_close(&trans);

    return true;
}

bool nas_qos_port_egress_map_delete(uint32_t nas_port_id)
{
    cps_api_transaction_params_t trans;

    cout << "cleanup all maps created for test" << endl;

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) {
        return false;
    }

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL) {
        return false;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
            BASE_QOS_PORT_EGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_EGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &nas_port_id, sizeof(uint32_t));
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_EGRESS_TC_TO_QUEUE_MAP, 0LL);
#ifndef DISABLE_TC_COLOR_MAP
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DOT1P_MAP, 0LL);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_EGRESS_TC_COLOR_TO_DSCP_MAP, 0LL);
#endif

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) {
        return false;
    }

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) {
        return false;
    }

    cps_api_transaction_close(&trans);

    for (int id = 0; map_info_list[id].name != NULL; id ++) {
        if (!nas_qos_map_delete(map_info_list[id].obj_attr,
                        map_info_list[id].map_id_attr, map_info_list[id].name,
                        *map_info_list[id].data)) {
            cout << "Failed to detete " << map_info_list[id].name << endl;
        }
    }

    return true;
}

TEST(cps_api_events, init) {
    char if_name[32];
    if (!nas_qos_get_first_phy_port(if_name, sizeof(if_name), test_port_id)) {
        return;
    }
    printf("front_port %s ifindex %u\n", if_name, test_port_id);
    ASSERT_TRUE(nas_qos_port_egress_map_create());
    ASSERT_TRUE(nas_qos_port_egress_map_entry_create());
    // Set test one particular queue
    ASSERT_TRUE(nas_qos_port_egress_set_test(test_port_id));
    ASSERT_TRUE(nas_qos_port_egress_get_test(test_port_id));
    ASSERT_TRUE(nas_qos_port_egress_map_entry_delete());
    ASSERT_TRUE(nas_qos_port_egress_map_delete(test_port_id));
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
