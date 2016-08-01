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
import nas_qos_utils as utl
import nas_os_utils as os_utl


def cps_trans_wr(l):
    print l
    return cps_utils.CPSTransaction(l).commit()


class IngPortCPSObj:

    """
    Utility class to build a CPS Object based on the QoS Ingress Port Yang model
    with attributes populated from the values passed as inputs.
    The resulting CPS Object representation, obtained by calling data() method,
    can be plugged into a CPS Transaction (by adding create,delete or set op)
    or directly into a CPS Get request.

    An instance of this class can also act as a wrapper over the CPS object
    dictionary returned by CPS Create or Get on Port Ingress Yang model and
    provides methods to extract various attributes from it.
    """
    yang_name = 'port-ingress'

    type_map = {
        'switch-id': ('leaf', 'uint32_t'),
        'port-id': ('leaf', 'uint32_t'),
        'default-traffic-class': ('leaf', 'uint8_t'),
        'dot1p-to-tc-map': ('leaf', 'uint64_t'),
        'dot1p-to-color-map': ('leaf', 'uint64_t'),
        'dot1p-to-tc-color-map': ('leaf', 'uint64_t'),
        'dscp-to-tc-map': ('leaf', 'uint64_t'),
        'dscp-to-color-map': ('leaf', 'uint64_t'),
        'dscp-to-tc-color-map': ('leaf', 'uint64_t'),
        'tc-to-queue-map': ('leaf', 'uint64_t'),
        'flow-control': ('leaf', 'enum', 'base-qos:flow-control:'),
        'policer_id': ('leaf', 'uint64_t'),
        'flood_storm_control': ('leaf', 'uint64_t'),
        'broadcast_storm_control': ('leaf', 'uint64_t'),
        'multicast_storm_control': ('leaf', 'uint64_t'),
        'priority_group_number': ('leaf', 'uint32_t'),
        'priority_group_id_list': ('leaf', 'uint64_t'),
        'per_priority_flow_control': ('leaf', 'uint8_t'),
        'tc-to-priority-group-map': ('leaf', 'uint64_t'),
        'priority_group_number': ('leaf', 'uint32_t'),
        'per_priority_flow_control': ('leaf', 'uint8_t'),
    }

    @classmethod
    def get_type_map(cls):
        return cls.type_map

    def __init__(self, ifindex=None, ifname=None, cps_data=None, switch_id=0,
                 list_of_attr_value_pairs=[]):

        if cps_data is not None:
            self.cps_data = cps_data
            return

        self.cps_data = None
        self.cps_obj_wr = utl.CPSObjWrp(self.yang_name, self.get_type_map())

        if ifindex is not None:
            self.cps_obj_wr.add_leaf_attr('port-id', ifindex)
        elif ifname is not None:
            ifindex = os_utl.if_nametoindex(ifname)
            if ifindex is None:
                raise RuntimeError("Port " + ifname + " not available")
            self.cps_obj_wr.add_leaf_attr('port-id', ifindex)

        self.cps_obj_wr.add_leaf_attr('switch-id', switch_id)

        for pair in list_of_attr_value_pairs:
            self.set_attr(pair[0], pair[1])

    def attrs(self):
        return self.cps_obj_wr.get_attrs()

    def set_attr(self, attr_name, attr_val):
        self.cps_obj_wr.add_leaf_attr(attr_name, attr_val)

    def data(self):
        if self.cps_data is not None:
            return self.cps_data

        return self.cps_obj_wr.get()

    def print_obj(self):
        """
        Print the contents of the CPS Object in a user friendly format
        """
        utl.print_obj(self, self.cps_data)

    def extract_attr(self, attr_name):
        """
        Get value for any attribute from the CPS data returned by Get
        """
        return utl.extract_cps_attr(self, self.cps_data, attr_name)


class EgPortCPSObj:

    """
    Utility class to build a CPS Object based on the QoS Egress Port Yang model
    with attributes populated from the values passed as inputs.
    The resulting CPS Object representation, obtained by calling data() method,
    can be plugged into a CPS Transaction (by adding create,delete or set op)
    or directly into a CPS Get request.

    An instance of this class can also act as a wrapper over the CPS object
    dictionary returned by CPS Create or Get on Port Egress Yang model and
    provides methods to extract various attributes from it.
    """

    yang_name = 'port-egress'

    type_map = {
        'switch-id': ('leaf', 'uint32_t'),
        'port-id': ('leaf', 'uint32_t'),
        'buffer-limit': ('leaf', 'uint64_t'),
        'drop-type': ('leaf', 'enum', 'base-qos:packet-drop-type'),
        'wred-profile-id': ('leaf', 'uint64_t'),
        'scheduler-profile-id': ('leaf', 'uint64_t'),
        'num-unicast-queue': ('leaf', 'uint8_t'),
        'num-multicast-queue': ('leaf', 'uint8_t'),
        'num-queue': ('leaf', 'uint8_t'),
        'tc-to-queue-map': ('leaf', 'uint64_t'),
        'tc-to-dot1p-map': ('leaf', 'uint64_t'),
        'tc-to-dscp-map': ('leaf', 'uint64_t'),
        'tc-color-to-dot1p-map': ('leaf', 'uint64_t'),
        'tc-color-to-dscp-map': ('leaf', 'uint64_t'),
        'pfc-priority-to-queue-map': ('leaf', 'uint64_t'),
    }

    @classmethod
    def get_type_map(cls):
        return cls.type_map

    def __init__(self, ifindex=None, ifname=None, cps_data=None, switch_id=0,
                 list_of_attr_value_pairs=[]):

        if cps_data is not None:
            self.cps_data = cps_data
            return

        self.cps_data = None
        self.cps_obj_wr = utl.CPSObjWrp(self.yang_name, self.get_type_map())

        if ifindex is not None:
            self.cps_obj_wr.add_leaf_attr('port-id', ifindex)
        elif ifname is not None:
            ifindex = os_utl.if_nametoindex(ifname)
            if ifindex is None:
                raise RuntimeError("Port " + ifname + " not available")
            self.cps_obj_wr.add_leaf_attr('port-id', ifindex)

        self.cps_obj_wr.add_leaf_attr('switch-id', switch_id)

        for pair in list_of_attr_value_pairs:
            self.set_attr(pair[0], pair[1])

    def attrs(self):
        return self.cps_obj_wr.get_attrs()

    def set_attr(self, attr_name, attr_val):
        self.cps_obj_wr.add_leaf_attr(attr_name, attr_val)

    def data(self):
        if self.cps_data is not None:
            return self.cps_data

        return self.cps_obj_wr.get()

    def print_obj(self):
        """
        Print the contents of the CPS Object in a user friendly format
        """
        utl.print_obj(self, self.cps_data)

    def extract_attr(self, attr_name):
        """
        Get value for any attribute from the CPS data returned by Get
        """
        return utl.extract_cps_attr(self, self.cps_data, attr_name)
