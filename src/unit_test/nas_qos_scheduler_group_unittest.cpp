
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
#include <string>
#include <unordered_map>

#include "cps_api_events.h"
#include "cps_api_key.h"
#include "cps_api_operation.h"
#include "cps_api_object.h"
#include "cps_api_errors.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"

#include "gtest/gtest.h"

#include "dell-base-qos.h"
#include "dell-base-phy-interface.h"
#include "nas_qos_ifindex_utl.h"

const uint_t ALL_SG_LEVEL = (uint_t)-1;

#define LEVEL_3 3
#define LEVEL_2 2
#define LEVEL_1 1
#define LEVEL_0 0

using namespace std;

bool nas_qos_scheduler_group_create_test(uint32_t port_id, uint64_t &sg_id,
                                         uint32_t level)
{
    printf("starting nas_qos_scheduler_group_create_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_GROUP_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_GROUP_PORT_ID, port_id);
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_GROUP_LEVEL, level);

    if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
        cout << "CPS API CREATE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    //Upon success, store the returned scheduler_group id for future retrieval
    cps_api_object_t recvd_obj = cps_api_object_list_get(tran.prev,0);
    cps_api_object_attr_t sg_attr = cps_api_get_key_data(recvd_obj, BASE_QOS_SCHEDULER_GROUP_ID);

    if (sg_attr == NULL) {
        cout << "Key  sg id is not returned\n";
        return false;
    }

    sg_id = cps_api_object_attr_data_u64(sg_attr);

    cout << " NAS returns scheduler_group Id: "<< sg_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_scheduler_group_get_test(uint64_t sg_id_key) {
    printf("starting nas_qos_scheduler_group_get_test\n");

    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK)
        return false;


    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_GROUP_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID,
            cps_api_object_ATTR_T_U64,
            &sg_id_key, sizeof(uint64_t));

    bool rc = false;

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);

        for (; ix < mx; ++ix) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list, ix);
            cps_api_object_attr_t sg_id = cps_api_get_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID);
            if (sg_id == NULL) {
                printf("No scheduler-group id found, idx=%lu\n", ix);
                continue;
            }
            cps_api_object_attr_t port_id = cps_api_object_attr_get(obj,
                                                            BASE_QOS_SCHEDULER_GROUP_PORT_ID);
            cps_api_object_attr_t level = cps_api_object_attr_get(obj,
                                                            BASE_QOS_SCHEDULER_GROUP_LEVEL);
            cps_api_object_attr_t scheduler_profile = cps_api_object_attr_get(obj,
                                                            BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID);
            cps_api_object_attr_t child_count = cps_api_object_attr_get(obj,
                                                            BASE_QOS_SCHEDULER_GROUP_CHILD_COUNT);
            vector<uint64_t> child_list;
            cps_api_object_it_t it;
            for (cps_api_object_it_begin(obj, &it); cps_api_object_it_valid(&it);
                 cps_api_object_it_next(&it)) {
                cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
                if (id == BASE_QOS_SCHEDULER_GROUP_CHILD_LIST) {
                    child_list.push_back(cps_api_object_attr_data_u64(it.attr));
                }
            }

            cout << "scheduler-group id: " << cps_api_object_attr_data_u64(sg_id) << endl;
            if (port_id)
                cout << "port id: "<< cps_api_object_attr_data_u32(port_id) <<endl;

            if (level)
                cout << "level: "<< cps_api_object_attr_data_u32(level) << endl;

            if (scheduler_profile)
                printf("scheduler profile: 0x%016lX\n", cps_api_object_attr_data_u64(scheduler_profile));

            if (child_count)
                cout << "child count: "<< cps_api_object_attr_data_u32(child_count) << endl;

            if (child_list.size() > 0) {
                uint_t idx;
                printf("child_list: ");
                for (idx = 0; idx < child_list.size(); idx ++) {
                    printf("0x%016lx ", child_list[idx]);
                }
                printf("\n");
            }

        }

        rc = true;
    }

    cps_api_get_request_close(&gp);
    return rc;
}

bool nas_qos_scheduler_group_delete_test(uint64_t sg_id) {


    printf("starting nas_qos_scheduler_group_delete_test\n");

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_GROUP_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID,
            cps_api_object_ATTR_T_U64,
            &sg_id, sizeof(uint64_t));

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


bool nas_qos_scheduler_group_set_child_list_test(uint64_t sg_id, uint32_t count, uint64_t *child_list) {
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_scheduler_group_set_child_list_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_GROUP_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID,
            cps_api_object_ATTR_T_U64,
            &sg_id, sizeof(uint64_t));

    if (count > 0) {
        for (uint32_t i = 0; i<count; i++) {
            cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_GROUP_CHILD_LIST, child_list[i]);
        }
    } else {
        cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_GROUP_CHILD_LIST, 0LL);
    }

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) return false;
    cps_api_transaction_close(&trans);

    return true;
}

bool nas_qos_get_queue_id_list(uint32_t ifindex, uint32_t type_key,
                               vector<uint64_t>& queue_id_list)
{
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

    cps_api_set_key_data(obj, BASE_QOS_QUEUE_TYPE,
            cps_api_object_ATTR_T_U32,
            &type_key, sizeof(uint32_t));

    if (cps_api_get(&gp) != cps_api_ret_code_OK) {
        cout << "Failed to get queue list" << endl;
        return false;
    }

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
            queue_id_list.push_back(val64);
        }

        if (wred_id)
            cout << " wred id: "<< cps_api_object_attr_data_u64(wred_id);

        if (scheduler_id)
            cout << " scheduler profile: "<< cps_api_object_attr_data_u64(scheduler_id);

        if (wred_id || scheduler_id)
            printf("\n");
    }

    cps_api_get_request_close(&gp);

    return true;
}

static bool nas_qos_scheduler_create_test(uint64_t& sched_id, uint32_t weight,
                                    uint32_t min_rate, uint32_t min_burst,
                                    uint32_t max_rate, uint32_t max_burst)
{
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
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_WEIGHT, weight);
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_PROFILE_METER_TYPE, BASE_QOS_METER_TYPE_PACKET);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MIN_RATE, min_rate);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MIN_BURST, min_burst);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MAX_RATE, max_rate);
    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_PROFILE_MAX_BURST, max_burst);

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

    cout <<  " NAS returns scheduler Id: "<< sched_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

static bool nas_qos_scheduler_delete_test(uint64_t sched_id)
{
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

bool nas_qos_scheduler_group_set_sched_test(uint64_t sg_id, uint64_t sched_id)
{
    cps_api_transaction_params_t trans;

    printf("starting nas_qos_scheduler_group_set_shed_test\n");

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_SCHEDULER_GROUP_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_SCHEDULER_GROUP_ID,
            cps_api_object_ATTR_T_U64,
            &sg_id, sizeof(uint64_t));

    cps_api_object_attr_add_u64(obj, BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID, sched_id);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK)
        return false;

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK)
        return false;
    cps_api_transaction_close(&trans);

    return true;
}

typedef struct scheduler_group_info_struct {
    uint64_t sg_id;
    uint32_t port_id;
    uint32_t level;
    uint64_t sched_id;
} scheduler_group_info_t;

bool nas_qos_get_scheduler_group_list(uint32_t ifindex, uint_t level,
                                      vector<scheduler_group_info_t>& sg_list)
{
    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK) {
        return false;
    }
    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL) {
        return false;
    }
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_QOS_SCHEDULER_GROUP_OBJ,
                                    cps_api_qualifier_TARGET);
    cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_GROUP_PORT_ID, ifindex);
    if (level != ALL_SG_LEVEL) {
        cps_api_object_attr_add_u32(obj, BASE_QOS_SCHEDULER_GROUP_LEVEL, level);
    }
    bool rc = false;
    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        size_t ix;
        size_t mx = cps_api_object_list_size(gp.list);
        for (ix = 0; ix < mx; ix ++) {
            scheduler_group_info_t sg_info;
            obj = cps_api_object_list_get(gp.list, ix);
            sg_info.port_id = ifindex;
            cps_api_object_attr_t sg_id_attr = cps_api_get_key_data(obj,
                                                    BASE_QOS_SCHEDULER_GROUP_ID);
            if (sg_id_attr == NULL) {
                printf("No scheduler-group id found, idx=%lu\n", ix);
                continue;
            }
            sg_info.sg_id = cps_api_object_attr_data_u64(sg_id_attr);
            cps_api_object_attr_t level_attr = cps_api_object_attr_get(obj,
                                                    BASE_QOS_SCHEDULER_GROUP_LEVEL);
            if (level_attr == NULL) {
                printf("No level found, idx=%lu\n", ix);
                continue;
            }
            sg_info.level = cps_api_object_attr_data_u32(level_attr);
            cps_api_object_attr_t sched_attr = cps_api_object_attr_get(obj,
                                                    BASE_QOS_SCHEDULER_GROUP_SCHEDULER_PROFILE_ID);
            if (sched_attr == NULL) {
                sg_info.sched_id = 0LL;
            } else {
                sg_info.sched_id = cps_api_object_attr_data_u64(sched_attr);
            }
            sg_list.push_back(sg_info);
        }
        rc = true;
    }

    cps_api_get_request_close(&gp);
    return rc;
}

bool nas_qos_scheduler_group_get_all_test(uint32_t ifindex)
{
    vector<scheduler_group_info_t> sg_list;

    if (nas_qos_get_scheduler_group_list(ifindex, ALL_SG_LEVEL,
                                         sg_list) == false) {
        return false;
    }
    printf("### Show list of total %lu scheduler-group:\n", sg_list.size());
    for (auto sg_info: sg_list) {
        nas_qos_scheduler_group_get_test(sg_info.sg_id);
    }

    return true;
}

static bool nas_qos_scheduler_group_cleanup_test(uint32_t ifindex, uint_t level,
                                        std::unordered_map<uint64_t, uint64_t>& sched_list,
                                        bool restore = false)
{
    vector<scheduler_group_info_t> sg_list;

    if (nas_qos_get_scheduler_group_list(ifindex, level,
                                         sg_list) == false) {
        printf("Failed to get scheduler-group ID list of level %u\n", level);
        return false;
    }
    bool rc;
    for (auto& sg_info: sg_list) {
        if (level != 0 && sg_info.sched_id != 0LL) {
            rc = nas_qos_scheduler_group_set_sched_test(sg_info.sg_id, 0LL);
            if (rc == false) {
                printf("Failed to cleanup scheduler-profile of scheduler-group id %lu\n",
                       sg_info.sg_id);
                return false;
            }
            if (restore) {
                if (sched_list.find(sg_info.sg_id) != sched_list.end()) {
                    uint64_t sched_id = sched_list[sg_info.sg_id];
                    rc = nas_qos_scheduler_group_set_sched_test(sg_info.sg_id, sched_id);
                    if (rc == false) {
                        printf("Failed to restore original scheduler profile %lu\n", sched_id);
                        return false;
                    }
                } else {
                    printf("Scheduler-group %lu is not in restore list\n", sg_info.sg_id);
                }
            } else {
                sched_list.insert(std::pair<uint64_t, uint64_t>
                                  (sg_info.sg_id, sg_info.sched_id));
            }
        }
    }
    return true;
}

TEST(cps_api_events, init) {
    uint32_t test_port_id;
    char test_port_name[32];
    uint64_t sched_id_l0, sched_id_l1;

    ASSERT_TRUE(nas_qos_get_first_phy_port(test_port_name, sizeof(test_port_name),
                                             test_port_id));
    printf("using front port %s ifindex %u\n", test_port_name, test_port_id);

    std::unordered_map<uint64_t, uint64_t> sched_id_list;
    ASSERT_TRUE(nas_qos_scheduler_group_cleanup_test(test_port_id, LEVEL_2, sched_id_list));
    ASSERT_TRUE(nas_qos_scheduler_group_cleanup_test(test_port_id, LEVEL_1, sched_id_list));

    ASSERT_TRUE(nas_qos_scheduler_create_test(sched_id_l0, 50, 100, 100, 500, 100));
    vector<scheduler_group_info_t> sg_list;
    ASSERT_TRUE(nas_qos_get_scheduler_group_list(test_port_id, LEVEL_1, sg_list));
    for (auto& sg_info: sg_list) {
        ASSERT_TRUE(nas_qos_scheduler_group_set_sched_test(sg_info.sg_id, sched_id_l0));
    }
    sg_list.clear();
    ASSERT_TRUE(nas_qos_scheduler_create_test(sched_id_l1, 30, 50, 100, 200, 100));
    ASSERT_TRUE(nas_qos_get_scheduler_group_list(test_port_id, LEVEL_2, sg_list));
    for (auto& sg_info: sg_list) {
        ASSERT_TRUE(nas_qos_scheduler_group_set_sched_test(sg_info.sg_id, sched_id_l1));
    }

    // show all scheduler-group created
    ASSERT_TRUE(nas_qos_scheduler_group_get_all_test(test_port_id));

    // restore and cleanup
    ASSERT_TRUE(nas_qos_scheduler_group_cleanup_test(test_port_id, LEVEL_2,
                                                     sched_id_list, true));
    ASSERT_TRUE(nas_qos_scheduler_group_cleanup_test(test_port_id, LEVEL_1,
                                                     sched_id_list, true));
    ASSERT_TRUE(nas_qos_scheduler_delete_test(sched_id_l0));
    ASSERT_TRUE(nas_qos_scheduler_delete_test(sched_id_l1));
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
 *  * nas_qos_scheduler_group_set_npu_and_attr_test requires proper npu ids to exist.
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
