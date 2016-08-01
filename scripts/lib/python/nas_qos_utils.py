#!/usr/bin/python
#
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN #AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
#  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#

import cps_utils
import cps
import cps_object
import bytearray_utils as ba_utils

obj_root = 'base-qos'
dbg_on = False

class CPSObjWrp:

    def __init__(self, yang_obj_name, type_map):
        self.cps_obj = cps_object.CPSObject(module=obj_path(yang_obj_name))
        self.type_map = type_map
        fill_cps_types(type_map, self.cps_obj)
        self.attrs = {}

    def add_leaf_attr(self, attr_name, attr_val, base_path=None):
        self.attrs[attr_name] = attr_val
        prfx = enum_prefix(self.type_map, attr_name)
        if prfx is not None:
            attr_val = enum_get(prfx + attr_val)
        if base_path is not None:
            attr_name = base_path + '/' + attr_name
        self.cps_obj.add_attr(attr_name, attr_val)

    def get(self):
        return self.cps_obj.get()

    def get_attrs(self):
        return self.attrs


def attr_mode(type_map, attr_name):
    if attr_name in type_map:
        return type_map[attr_name][0]
    return None


def attr_type(type_map, attr_name):
    if attr_name in type_map:
        return type_map[attr_name][1]
    return None


def enum_prefix(type_map, attr_name):
    if attr_name in type_map:
        if attr_type(type_map, attr_name) == 'enum':
            return type_map[attr_name][2]
    return None


def fill_cps_types(type_map, cps_obj):
    for attr_name in type_map:
        if attr_mode(type_map, attr_name) == 'leaf':
            atype = attr_type(type_map, attr_name)
            if atype == 'enum':
                cps_obj.add_attr_type(attr_name, 'uint32_t')
            else:
                cps_obj.add_attr_type(attr_name, atype)


def extract_cps_obj(cps_data):
    if 'change' in cps_data:
        return cps_data['change']['data']
    if 'data' in cps_data:
        return cps_data['data']
    return cps_data


def extract_cps_attr(qos_obj, cps_data, attr_name, yang_name=None):
    """
    Get value for attr from the CPS data returned by Create or Get

    @cps_data - CPS data returned by Create or Get
    """
    if yang_name is None:
        yang_name = qos_obj.yang_name

    type_map = qos_obj.get_type_map()

    cps_obj = extract_cps_obj(cps_data)
    path = name_to_path(yang_name, attr_name)

    if 'cps/key_data' in cps_obj and \
            path in cps_obj['cps/key_data']:
        return extract_cps_attr(qos_obj, cps_obj['cps/key_data'], attr_name)

    if path not in cps_obj:
        return None

    val = cps_obj[path]

    prfx = enum_prefix(type_map, attr_name)
    atype = attr_type(type_map, attr_name)

    if prfx is not None:
        der_val = ba_utils.from_ba(val, 'uint32_t')
        der_val = enum_reverse_get(der_val, enum_prefix(type_map, attr_name))
    elif atype is not None:
        if isinstance(val, list):
            der_val = []
            for ba_val in val:
                attr_val = ba_utils.from_ba(ba_val, atype)
                der_val.append(attr_val)
        else:
            der_val = ba_utils.from_ba(val, atype)
    else:
        return val
    return der_val


def print_obj(qos_obj, cps_data):
    print cps_data
    for attr_name in qos_obj.get_type_map():
        val = extract_cps_attr(qos_obj, cps_data, attr_name)
        if val is not None:
            print attr_name, " " * (30 - len(attr_name)), ": ", str(val)
    print '-' * 40


def name_to_path(yang_obj_name, attr_name):
    return obj_path(yang_obj_name) + '/' + attr_name


def obj_path(yang_obj_name):
    return obj_root + '/' + yang_obj_name


def enum_map_get():
    enum_map = {
        'base-qos:meter-type:PACKET': 1,
        'base-qos:meter-type:BYTE': 2,

        'base-qos:meter-mode:Sr_TCM': 1,
        'base-qos:meter-mode:Tr_TCM': 2,
        'base-qos:meter-mode:Sr_TWO_COLOR': 3,
        'base-qos:meter-mode:STORM_CONTROL': 4,

        'base-qos:meter-color-source:BLIND': 1,
        'base-qos:meter-color-source:AWARE': 2,

        'base-qos:policer-action:FORWARD': 1,
        'base-qos:policer-action:DROP': 2,

        'base-qos:packet-color:GREEN': 1,
        'base-qos:packet-color:YELLOW': 2,
        'base-qos:packet-color:RED': 3,

        'base-qos:queue-type:NONE': 1,
        'base-qos:queue-type:UCAST': 2,
        'base-qos:queue-type:MULTICAST': 3,

        'base-qos:packet-drop-type:TAIL': 1,
        'base-qos:packet-drop-type:WRED': 2,

        'base-qos:scheduling-type:SP': 1,
        'base-qos:scheduling-type:WRR': 2,
        'base-qos:scheduling-type:WDRR': 3,

        'base-qos:flow-control:DISABLE': 1,
        'base-qos:flow-control:TX-ONLY': 2,
        'base-qos:flow-control:RX-ONLY': 3,
        'base-qos:flow-control:BOTH-ENABLE': 4,

        'base-qos:buffer-pool-type:INGRESS': 1,
        'base-qos:buffer-pool-type:EGRESS': 2,

        'base-qos:buffer-threshold-mode:STATIC': 1,
        'base-qos:buffer-threshold-mode:DYNAMIC': 2,



    }
    return enum_map


def enum_reverse_get(val, enum_type):
    for key in enum_map_get():
        if key.split(':')[0] == enum_type.split(':')[0] and \
           key.split(':')[1] == enum_type.split(':')[1]:
            if enum_get(key) == val:
                return key.split(':')[2]


def enum_get(enum_str):
    return enum_map_get()[enum_str]


def dbg_print(*args):
    if dbg_on:
        print args
