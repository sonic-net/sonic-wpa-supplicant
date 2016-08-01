
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
#include <string>
#include <unordered_map>
#include <algorithm>
#include <list>

#include "cps_api_events.h"
#include "cps_api_key.h"
#include "cps_api_operation.h"
#include "cps_api_object.h"
#include "cps_api_errors.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"
#include "dell-base-qos.h"
#include "dell-base-phy-interface.h"

using namespace std;

typedef unordered_map<string, uint32_t> if_info_map_t;
const uint32_t INTERFACE_TYPE_ALL = (uint32_t)-1;

static bool nas_qos_get_intf_list(if_info_map_t& if_list, uint32_t if_type)
{
    cps_api_get_params_t gp;
    if (cps_api_get_request_init(&gp) != cps_api_ret_code_OK) {
        return false;
    }

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    if (obj == NULL) {
        return false;
    }
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_PORT_INTERFACE_OBJ,
            cps_api_qualifier_TARGET);

    if (cps_api_get(&gp) == cps_api_ret_code_OK) {
        cps_api_object_attr_t attr;
        uint32_t ifindex;
        uint32_t type_val;
        char *ifname;
        size_t obj_num = cps_api_object_list_size(gp.list);
        for (size_t id = 0; id < obj_num; id ++) {
            obj = cps_api_object_list_get(gp.list, id);
            attr = cps_api_get_key_data(obj, BASE_PORT_INTERFACE_IFINDEX);
            if (!attr) {
                continue;
            }
            ifindex = cps_api_object_attr_data_u32(attr);
            attr = cps_api_object_attr_get(obj, BASE_PORT_INTERFACE_NAME);
            if (!attr) {
                continue;
            }
            ifname = (char *)cps_api_object_attr_data_bin(attr);
            attr = cps_api_object_attr_get(obj, BASE_PORT_INTERFACE_INTERFACE_TYPE);
            if (!attr) {
                continue;
            }
            type_val = cps_api_object_attr_data_u32(attr);
            if (if_type != INTERFACE_TYPE_ALL && if_type != type_val) {
                continue;
            }
            if_list.insert(make_pair(ifname, ifindex));
        }
    }

    return true;
}

bool nas_qos_get_if_index(const char *if_name, uint32_t& if_index)
{
    if_info_map_t if_list;
    if (!nas_qos_get_intf_list(if_list, INTERFACE_TYPE_ALL)) {
        printf("failed to get interface list\n");
        return false;
    }
    if_info_map_t::iterator pos = if_list.find(if_name);
    if (pos == if_list.end()) {
        printf("interface %s not found\n", if_name);
        return false;
    }
    if_index = pos->second;
    printf("if_index of %s: %d\n", if_name, if_index);
    return true;
}

bool nas_qos_get_first_phy_port(char *if_name, uint_t name_len,
                                uint32_t& if_index)
{
    if_info_map_t if_list;
    if (!nas_qos_get_intf_list(if_list,
                               BASE_CMN_INTERFACE_TYPE_L2_PORT)) {
        printf("failed to get interface list\n");
        return false;
    }
    if (if_list.size() == 0) {
        printf("no physical port found\n");
        return false;
    }
    list<string> key_list;
    for (auto& if_info: if_list) {
        key_list.push_back(if_info.first);
    }
    key_list.sort();
    string name = key_list.front();
    if (if_name && name_len > 0) {
        strncpy(if_name, name.c_str(), name_len);
        if_name[name_len - 1] = '\0';
    }
    if_index = if_list[name];

    return true;
}
