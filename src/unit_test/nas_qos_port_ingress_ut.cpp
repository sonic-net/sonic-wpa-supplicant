
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
#include "cps_class_map.h"
#include "cps_api_object_key.h"
#include "nas_qos_common.h"

#include "gtest/gtest.h"

#include "dell-base-qos.h"
#include "nas_qos_ifindex_utl.h"

using namespace std;

#define MAX_TEST_PORT_ID     12
static uint32_t test_port_id;

uint64_t dot1p_tc_map_id, dot1p_color_map_id, dot1p_tc_color_map_id;
uint64_t dscp_tc_map_id, dscp_color_map_id, dscp_tc_color_map_id;
uint64_t tc_queue_map_id;
uint64_t test_meter_id;

static inline const char *
get_flow_control_name(uint32_t name_id)
{
    BASE_QOS_FLOW_CONTROL_t flow_ctl_id = (BASE_QOS_FLOW_CONTROL_t)name_id;
    switch (flow_ctl_id) {
    case BASE_QOS_FLOW_CONTROL_DISABLE:
        return "disable";
    case BASE_QOS_FLOW_CONTROL_TX_ONLY:
        return "tx-only";
    case BASE_QOS_FLOW_CONTROL_RX_ONLY:
        return "rx-only";
    case BASE_QOS_FLOW_CONTROL_BOTH_ENABLE:
        return "both";
    default:
        return "unknown";
    }
}

bool nas_qos_port_ingress_get_test(uint32_t nas_port_id)
{
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
            BASE_QOS_PORT_INGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_INGRESS_PORT_ID,
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
                return false;
            }
            attr = cps_api_get_key_data(obj, BASE_QOS_PORT_INGRESS_PORT_ID);
            if (attr == NULL) {
                cout << "failed to get port id, obj_id=" << idx << endl;
                return false;
            }
            uint32_t port_id = cps_api_object_attr_data_u32(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_DEFAULT_TRAFFIC_CLASS);
            if (attr == NULL) {
                cout << "failed to get default traffic class, obj_id=" << idx << endl;
                return false;
            }
            uint32_t default_tc = cps_api_object_attr_data_u32(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_MAP);
            if (attr == NULL) {
                cout << "failed to get dot1p to tc map, obj_id=" << idx << endl;
                return false;
            }
            uint64_t dot1p_to_tc = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_COLOR_MAP);
            if (attr == NULL) {
                cout << "failed to get dot1p to color map, obj_id=" << idx << endl;
                return false;
            }
            uint64_t dot1p_to_color = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP);
            if (attr == NULL) {
                cout << "failed to get dot1p to tc color map, obj_id=" << idx << endl;
                return false;
            }
            uint64_t dot1p_to_tc_color = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_DSCP_TO_TC_MAP);
            if (attr == NULL) {
                cout << "failed to get dscp to tc map, obj_id=" << idx << endl;
                return false;
            }
            uint64_t dscp_to_tc = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_DSCP_TO_COLOR_MAP);
            if (attr == NULL) {
                cout << "failed to get dscp to color map, obj_id=" << idx << endl;
                return false;
            }
            uint64_t dscp_to_color = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP);
            if (attr == NULL) {
                cout << "failed to get dscp to tc color map, obj_id=" << idx << endl;
                return false;
            }
            uint64_t dscp_to_tc_color = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP);
            if (attr == NULL) {
                cout << "failed to get tc to queue map, obj_id=" << idx << endl;
                return false;
            }
            uint64_t tc_to_queue = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_FLOW_CONTROL);
            if (attr == NULL) {
                cout << "failed to get flow control, obj_id=" << idx << endl;
                return false;
            }
            uint32_t flow_control = cps_api_object_attr_data_u32(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_POLICER_ID);
            if (attr == NULL) {
                cout << "failed to get policer id, obj_id=" << idx << endl;
                return false;
            }
            uint64_t policer_id = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL);
            if (attr == NULL) {
                cout << "failed to get flood storm control profile, obj_id=" << idx << endl;
                return false;
            }
            uint64_t flood_storm_ctl = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL);
            if (attr == NULL) {
                cout << "failed to get broadcast storm control profile, obj_id=" << idx << endl;
                return false;
            }
            uint64_t bcast_storm_ctl = cps_api_object_attr_data_u64(attr);

            attr = cps_api_object_attr_get(obj, BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL);
            if (attr == NULL) {
                cout << "failed to get multicast storm control profile, obj_id=" << idx << endl;
                return false;
            }
            uint64_t mcast_storm_ctl = cps_api_object_attr_data_u64(attr);

            if (dot1p_to_tc_color != dot1p_tc_color_map_id) {
                cout << "dot1p_to_tc_color map id mismatch: " <<
                    "exp " << dot1p_tc_color_map_id <<
                    " got " << dot1p_to_tc_color << endl;
                return false;
            }

            if (dscp_to_tc_color != dscp_tc_color_map_id) {
                cout << "dscp_to_tc_color map id mismatch: " <<
                    "exp " << dscp_tc_color_map_id <<
                    " got " << dscp_to_tc_color << endl;
                return false;
            }

            cout << "Object " << idx << endl;
            cout << "  port                    : " << port_id << endl;
            cout << "  deault traffic class    : " << default_tc << endl;
            cout << "  dot1p to tc map         : " << dot1p_to_tc << endl;
            cout << "  dot1p to color map      : " << dot1p_to_color << endl;
            cout << "  dot1p to tc color map   : " << dot1p_to_tc_color << endl;
            cout << "  dscp to tc map          : " << dscp_to_tc << endl;
            cout << "  dscp to color map       : " << dscp_to_color << endl;
            cout << "  dscp to tc color map    : " << dscp_to_tc_color << endl;
            cout << "  tc to queue map         : " << tc_to_queue << endl;
            cout << "  flow control            : " << get_flow_control_name(flow_control)
                << endl;
            cout << "  policer id              : " << policer_id << endl;
            cout << "  flood storm control     : " << flood_storm_ctl << endl;
            cout << "  broadcast storm control : " << bcast_storm_ctl << endl;
            cout << "  multicast storm control : " << mcast_storm_ctl << endl;
            cout << endl;
        }
    }

    return true;
}

typedef struct {
    nas_qos_map_entry_key_t key;
    nas_qos_map_entry_value_t val;
} map_entry_info_t;

map_entry_info_t dot1p_to_tc_color_entries[] = {
    {{1}, {2, BASE_QOS_PACKET_COLOR_GREEN}},
    {{2}, {5, BASE_QOS_PACKET_COLOR_YELLOW}},
    {{3}, {6, BASE_QOS_PACKET_COLOR_RED}},
};

int dot1p_to_tc_color_entry_cnt = sizeof(dot1p_to_tc_color_entries)/sizeof(map_entry_info_t);

map_entry_info_t dscp_to_tc_color_entries[] = {
    {{1}, {1, BASE_QOS_PACKET_COLOR_GREEN}},
    {{2}, {2, BASE_QOS_PACKET_COLOR_YELLOW}},
    {{3}, {3, BASE_QOS_PACKET_COLOR_GREEN}},
};

int dscp_to_tc_color_entry_cnt = sizeof(dot1p_to_tc_color_entries)/sizeof(map_entry_info_t);

map_entry_info_t tc_to_queue_entries[] = {
    {{BASE_QOS_QUEUE_TYPE_UCAST << 16 | 0}, {0, (BASE_QOS_PACKET_COLOR_t)0, 0, 0, 0}},
    {{BASE_QOS_QUEUE_TYPE_UCAST << 16 | 2}, {0, (BASE_QOS_PACKET_COLOR_t)0, 0, 0, 2}},
    {{BASE_QOS_QUEUE_TYPE_UCAST << 16 | 3}, {0, (BASE_QOS_PACKET_COLOR_t)0, 0, 0, 3}},
    {{BASE_QOS_QUEUE_TYPE_MULTICAST << 16 | 0}, {0, (BASE_QOS_PACKET_COLOR_t)0, 0, 0, 0}},
    {{BASE_QOS_QUEUE_TYPE_MULTICAST << 16 | 2}, {0, (BASE_QOS_PACKET_COLOR_t)0, 0, 0, 2}},
    {{BASE_QOS_QUEUE_TYPE_MULTICAST << 16 | 3}, {0, (BASE_QOS_PACKET_COLOR_t)0, 0, 0, 3}},
};

int tc_to_queue_entry_cnt = sizeof(tc_to_queue_entries)/sizeof(map_entry_info_t);

static struct {
    const char *name;
    cps_api_attr_id_t obj_attr;
    cps_api_attr_id_t entry_attr;
    cps_api_attr_id_t map_id_attr;
    uint64_t *data;
    map_entry_info_t *entries;
    int entry_cnt;
} map_info_list[] = {
    {"dot1p_to_tc_color", BASE_QOS_DOT1P_TO_TC_COLOR_MAP_OBJ, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY,
        BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ID, &dot1p_tc_color_map_id,
        dot1p_to_tc_color_entries, dot1p_to_tc_color_entry_cnt},
    {"dscp_to_tc_color", BASE_QOS_DSCP_TO_TC_COLOR_MAP_OBJ, BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY,
        BASE_QOS_DSCP_TO_TC_COLOR_MAP_ID, &dscp_tc_color_map_id,
        dscp_to_tc_color_entries, dscp_to_tc_color_entry_cnt},
    {"tc_to_queue", BASE_QOS_TC_TO_QUEUE_MAP_OBJ, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY,
        BASE_QOS_TC_TO_QUEUE_MAP_ID, &tc_queue_map_id,
        tc_to_queue_entries, tc_to_queue_entry_cnt},
    {NULL, 0, 0, 0, NULL, NULL, 0}
};

static bool nas_qos_map_create_entries(cps_api_attr_id_t obj_attr_id,
                                       cps_api_attr_id_t map_attr_id, uint64_t map_id,
                                       map_entry_info_t *entries, int entry_cnt)
{
    int idx;

    cps_api_transaction_params_t tran;
    cps_api_object_t obj;
    if (!entries) {
        return true;
    }
    for (idx = 0; idx < entry_cnt; idx ++) {
        cout << "Creating map entry for map " << map_id << endl;
        if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK) {
            return false;
        }
        obj = cps_api_object_create();
        cps_api_key_from_attr_with_qual(cps_api_object_key(obj), obj_attr_id,
                                        cps_api_qualifier_TARGET);
        cps_api_set_key_data(obj, map_attr_id,
                             cps_api_object_ATTR_T_U64,
                             &map_id, sizeof(uint64_t));
        if (obj_attr_id == BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY) {
            uint8_t dot1p = entries[idx].key.dot1p;
            uint8_t tc = entries[idx].val.tc;
            uint32_t color = entries[idx].val.color;
            cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_DOT1P,
                                 cps_api_object_ATTR_T_BIN,
                                 &dot1p, sizeof(uint8_t));
            cps_api_object_attr_add(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_TC,
                                    &tc, 1);
            cps_api_object_attr_add_u32(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_COLOR,
                                        color);
        } else if (obj_attr_id == BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY) {
            uint8_t dscp = entries[idx].key.dot1p;
            uint8_t tc = entries[idx].val.tc;
            uint32_t color = entries[idx].val.color;
            cps_api_set_key_data(obj, BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_DSCP,
                                 cps_api_object_ATTR_T_BIN,
                                 &dscp, sizeof(uint8_t));
            cps_api_object_attr_add(obj, BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_TC,
                                    &tc, 1);
            cps_api_object_attr_add_u32(obj, BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_COLOR,
                                        color);
        } else if (obj_attr_id == BASE_QOS_TC_TO_QUEUE_MAP_ENTRY) {
            uint8_t tc = entries[idx].key.any & 0xffff;
            uint32_t type = (entries[idx].key.any >> 16) & 0xffff;
            uint32_t number = entries[idx].val.queue_num;
            cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TC,
                                 cps_api_object_ATTR_T_BIN,
                                 &tc, sizeof(uint8_t));
            cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TYPE,
                                 cps_api_object_ATTR_T_BIN,
                                 &type, sizeof(uint32_t));
            cps_api_object_attr_add_u32(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_QUEUE_NUMBER,
                                        number);
        } else {
            cout << "Unsupported map entry type to create" << endl;
            return false;
        }

        if (cps_api_create(&tran, obj) != cps_api_ret_code_OK) {
            return false;
        }

        if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
            cout << "Commit failed" << endl;
            return false;
        }

        if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
            return false;
        }
    }
    return true;
}

static bool nas_qos_map_create(cps_api_attr_id_t obj_attr_id, cps_api_attr_id_t entry_attr_id,
                            cps_api_attr_id_t map_attr_id,
                            map_entry_info_t *entries, int entry_cnt,
                            const char *map_name, uint64_t& map_id)
{
    cout << "Starting create " << map_name << " map" << endl;
    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

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
        cout << "Key dot1p_color_map id  not returned\n";
        return false;
    }

    map_id = cps_api_object_attr_data_u64(map_id_attr);

    cout << " NAS returns " << map_name << " Id: "<< map_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    if (!nas_qos_map_create_entries(entry_attr_id, map_attr_id,
                                    map_id, entries, entry_cnt)) {
        cout << "Failed to create map entries" << endl;
        return false;
    }

    return true;
}

bool nas_qos_port_ingress_map_create(void)
{
    for (int id = 0; map_info_list[id].name != NULL; id ++) {
        if (!nas_qos_map_create(map_info_list[id].obj_attr, map_info_list[id].entry_attr,
                        map_info_list[id].map_id_attr,
                        map_info_list[id].entries, map_info_list[id].entry_cnt,
                        map_info_list[id].name, *map_info_list[id].data)) {
            cout << "Failed to create " << map_info_list[id].name << endl;
            return false;
        }
    }

    return true;
}

static bool nas_qos_map_delete_entries(cps_api_attr_id_t obj_attr_id,
                                       cps_api_attr_id_t map_attr_id, uint64_t map_id,
                                       map_entry_info_t *entries, int entry_cnt)
{
    int idx;

    cps_api_transaction_params_t tran;
    cps_api_object_t obj;
    if (!entries) {
        return true;
    }
    for (idx = 0; idx < entry_cnt; idx ++) {
        cout << "Deleting map entry for map " << map_id << endl;
        if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK) {
            return false;
        }
        obj = cps_api_object_create();
        cps_api_key_from_attr_with_qual(cps_api_object_key(obj), obj_attr_id,
                                        cps_api_qualifier_TARGET);
        cps_api_set_key_data(obj, map_attr_id,
                             cps_api_object_ATTR_T_U64,
                             &map_id, sizeof(uint64_t));
        if (obj_attr_id == BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY) {
            uint8_t dot1p = entries[idx].key.dot1p;
            cps_api_set_key_data(obj, BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_DOT1P,
                                 cps_api_object_ATTR_T_BIN,
                                 &dot1p, sizeof(uint8_t));
        } else if (obj_attr_id == BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY) {
            uint8_t dscp = entries[idx].key.dot1p;
            cps_api_set_key_data(obj, BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_DSCP,
                                 cps_api_object_ATTR_T_BIN,
                                 &dscp, sizeof(uint8_t));
        } else if (obj_attr_id == BASE_QOS_TC_TO_QUEUE_MAP_ENTRY) {
            uint8_t tc = entries[idx].key.any & 0xffff;
            uint32_t type = (entries[idx].key.any >> 16) & 0xffff;
            cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TC,
                                 cps_api_object_ATTR_T_BIN,
                                 &tc, sizeof(uint8_t));
            cps_api_set_key_data(obj, BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_TYPE,
                                 cps_api_object_ATTR_T_BIN,
                                 &type, sizeof(uint32_t));
        } else {
            cout << "Unsupported map entry type to delete" << endl;
            return false;
        }

        if (cps_api_delete(&tran, obj) != cps_api_ret_code_OK) {
            return false;
        }

        if (cps_api_commit(&tran) != cps_api_ret_code_OK) {
            cout << "Commit failed" << endl;
            return false;
        }

        if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
            return false;
        }
    }
    return true;
}

static bool nas_qos_map_delete(cps_api_attr_id_t obj_attr_id, cps_api_attr_id_t entry_attr_id,
                            cps_api_attr_id_t map_attr_id,
                            map_entry_info_t *entries, int entry_cnt,
                            const char *map_name, uint64_t map_id)
{

    if (!nas_qos_map_delete_entries(entry_attr_id, map_attr_id,
                                    map_id, entries, entry_cnt)) {
        cout << "Failed to delete map entries" << endl;
        return false;
    }

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

bool nas_qos_port_ingress_policer_create(void)
{

    cout << "starting " << __FUNCTION__ << endl;

    cps_api_transaction_params_t tran;
    if (cps_api_transaction_init(&tran) != cps_api_ret_code_OK)
        return false;

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_TYPE, BASE_QOS_METER_TYPE_PACKET);
    cps_api_object_attr_add_u32(obj, BASE_QOS_METER_MODE, BASE_QOS_METER_MODE_STORM_CONTROL);
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
        cout << "Key  policer id  not returned\n";
        return false;
    }

    test_meter_id = cps_api_object_attr_data_u64(policer_attr);

    cout <<  " NAS returns policer Id: "<< test_meter_id  << endl;

    if (cps_api_transaction_close(&tran) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}

bool nas_qos_port_ingress_set_test(uint32_t nas_port_id)
{
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
            BASE_QOS_PORT_INGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_INGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &nas_port_id, sizeof(uint32_t));

    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP, dot1p_tc_color_map_id);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP, dscp_tc_color_map_id);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP, tc_queue_map_id);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL, test_meter_id);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL, test_meter_id);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL, test_meter_id);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) {
        return false;
    }

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) {
        return false;
    }

    cps_api_transaction_close(&trans);

    return true;
}

bool nas_qos_port_ingress_map_delete(uint32_t nas_port_id)
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
            BASE_QOS_PORT_INGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_INGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &nas_port_id, sizeof(uint32_t));
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DOT1P_TO_TC_COLOR_MAP, 0LL);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_DSCP_TO_TC_COLOR_MAP, 0LL);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_TC_TO_QUEUE_MAP, 0LL);

    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) {
        return false;
    }

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) {
        return false;
    }

    cps_api_transaction_close(&trans);

    for (int id = 0; map_info_list[id].name != NULL; id ++) {
        if (!nas_qos_map_delete(map_info_list[id].obj_attr, map_info_list[id].entry_attr,
                        map_info_list[id].map_id_attr,
                        map_info_list[id].entries, map_info_list[id].entry_cnt,
                        map_info_list[id].name, *map_info_list[id].data)) {
            cout << "Failed to detete " << map_info_list[id].name << endl;
        }
    }

    return true;
}

bool nas_qos_port_ingress_policer_delete(uint32_t nas_port_id)
{
    cps_api_transaction_params_t trans;

    cout << "cleanup policer created for test" << endl;

    if (cps_api_transaction_init(&trans)!=cps_api_ret_code_OK) {
        return false;
    }

    cps_api_object_t obj = cps_api_object_create();
    if (obj == NULL) {
        return false;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
            BASE_QOS_PORT_INGRESS_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_PORT_INGRESS_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &nas_port_id, sizeof(uint32_t));
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_FLOOD_STORM_CONTROL, 0LL);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_BROADCAST_STORM_CONTROL, 0LL);
    cps_api_object_attr_add_u64(obj, BASE_QOS_PORT_INGRESS_MULTICAST_STORM_CONTROL, 0LL);


    if (cps_api_set(&trans,obj)!=cps_api_ret_code_OK) {
        return false;
    }

    if (cps_api_commit(&trans)!=cps_api_ret_code_OK) {
        return false;
    }

    cps_api_transaction_close(&trans);

    cout << "starting delete policer" << endl;

    if (cps_api_transaction_init(&trans) != cps_api_ret_code_OK)
        return false;

    obj = cps_api_object_create();
    if (obj == NULL)
        return false;

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_METER_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_set_key_data(obj, BASE_QOS_METER_ID,
            cps_api_object_ATTR_T_U64,
            &test_meter_id, sizeof(uint64_t));

    if (cps_api_delete(&trans, obj) != cps_api_ret_code_OK) {
        cout << "CPS API DELETE FAILED" <<endl;
        return false;
    }

    if (cps_api_commit(&trans) != cps_api_ret_code_OK) {
        cout << "CPS API commit failed" <<endl;
        return false;
    }

    if (cps_api_transaction_close(&trans) != cps_api_ret_code_OK) {
        cout << "CPS API transaction closed" << endl;
        return false;
    }

    return true;
}


TEST(cps_api_events, init) {
    char if_name[32];
    if (!nas_qos_get_first_phy_port(if_name, sizeof(if_name), test_port_id)) {
        return;
    }
    printf("front_port %s ifindex %u\n", if_name, test_port_id);
    ASSERT_TRUE(nas_qos_port_ingress_map_create());
    ASSERT_TRUE(nas_qos_port_ingress_policer_create());
    // Set test one particular queue
    ASSERT_TRUE(nas_qos_port_ingress_set_test(test_port_id));
    ASSERT_TRUE(nas_qos_port_ingress_get_test(test_port_id));
    ASSERT_TRUE(nas_qos_port_ingress_policer_delete(test_port_id));
    ASSERT_TRUE(nas_qos_port_ingress_map_delete(test_port_id));
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
