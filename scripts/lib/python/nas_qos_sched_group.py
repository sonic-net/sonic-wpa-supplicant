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


class SchedGroupCPSObj:

    yang_name = 'scheduler-group'

    type_map = {
        'switch-id': ('leaf', 'uint32_t'),
        'id': ('leaf', 'uint64_t'),
        'port-id': ('leaf', 'uint32_t'),
        'level': ('leaf', 'uint32_t'),
        'scheduler-profile-id': ('leaf', 'uint64_t'),
        'child_count': ('leaf', 'uint32_t'),
        'child-list': ('leaf', 'uint64_t'),
    }

    @classmethod
    def get_type_map(cls):
        return cls.type_map

    def __init__(
        self, port_name=None, level=None, switch_id=0, sg_id=None,
            sched_id=None, chld_id_list=[], cps_data=None):
        if cps_data is not None:
            self.cps_data = cps_data
            return

        self.cps_obj_wr = utl.CPSObjWrp(self.yang_name, self.get_type_map())
        self.cps_data = self.cps_obj_wr.get()
        self.set_attr('switch-id', switch_id)
        if port_name is not None:
            port_id = os_utl.if_nametoindex(port_name)
            self.set_attr('port-id', port_id)
        if sg_id is not None:
            self.set_attr('id', sg_id)
        if level is not None:
            self.set_attr('level', level)
        if sched_id is not None:
            self.set_attr('scheduler-profile-id', sched_id)
        if chld_id_list is not None and isinstance(chld_id_list, list) and len(chld_id_list) > 0:
            self.set_attr('child_count', len(chld_id_list))
            self.set_attr('child-list', chld_id_list)

    def attrs(self):
        return self.cps_obj_wr.get_attrs()

    def set_attr(self, attr_name, attr_val):
        self.cps_obj_wr.add_leaf_attr(attr_name, attr_val)

    def data(self):
        return self.cps_data

    def print_obj(self):
        """
        Print the contents of the CPS Object in a user friendly format
        """
        print self.data()
        for attr_name in self.get_type_map():
            val = utl.extract_cps_attr(self, self.data(), attr_name)
            if val is not None:
                print attr_name, " " * (30 - len(attr_name)), ": ", str(val)

    def extract_id(self):
        """
        Get Scheduler profile ID from the CPS data returned by Create of Get
        """
        return utl.extract_cps_attr(self, self.cps_data, 'id')

    def extract_attr(self, attr_name):
        """
        Get value for any attribute from the CPS data returned by Get
        """
        return utl.extract_cps_attr(self, self.cps_data, attr_name)
