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


class QueueCPSObj:

    """
    Utility class to build a CPS Object based on the QoS Queue Yang model
    with attributes populated from the values passed as inputs.
    The resulting CPS Object representation, obtained by calling data() method,
    can be plugged into a CPS Transaction (by adding create,delete or set op)
    or directly into a CPS Get request.

    An instance of this class can also act as a wrapper over the CPS object
    dictionary returned by CPS Create or Get on Queue Yang model and
    provides methods to extract various attributes from it.
    """

    yang_name = 'queue'

    type_map = {
        'switch-id': ('leaf', 'uint32_t'),
        'port-id': ('leaf', 'uint32_t'),
        'type': ('leaf', 'enum', 'base-qos:queue-type:'),
        'queue-number': ('leaf', 'uint32_t'),
        'id': ('leaf', 'uint64_t'),
        'wred-id': ('leaf', 'uint64_t'),
        'buffer-profile-id': ('leaf', 'uint64_t'),
        'scheduler-profile-id': ('leaf', 'uint64_t'),
    }

    @classmethod
    def get_type_map(cls):
        return cls.type_map

    def __init__(self, queue_type, queue_number,
                 port_id, switch_id=0, cps_data=None):

        if cps_data is not None:
            self.cps_data = cps_data
            return

        self.cps_data = None
        self.cps_obj_wr = utl.CPSObjWrp(self.yang_name, self.get_type_map())

        if queue_type is not None:
            self.cps_obj_wr.add_leaf_attr('type', queue_type)
        if queue_number is not None:
            self.cps_obj_wr.add_leaf_attr('queue-number', queue_number)
        if port_id is not None:
            self.cps_obj_wr.add_leaf_attr('port-id', port_id)
        self.cps_obj_wr.add_leaf_attr('switch-id', switch_id)

    def attrs(self):
        return self.cps_obj_wr.get_attrs()

    def set_attr(self, attr_name, attr_val):
        self.cps_obj_wr.add_leaf_attr(attr_name, attr_val)

    def data(self):
        if self.cps_data is not None:
            return self.cps_data

        return self.cps_obj_wr.get()

    def extract_id(self):
        """
        Get Queue ID from the CPS data returned by Create or Get
        """
        return utl.extract_cps_attr(self, self.cps_data, 'id')

    def print_obj(self):
        """
        Print the contents of the CPS Object in a user friendly format
        """
        utl.print_obj(self, self.cps_data)

    def extract_opaque_data(self):
        """
        Get Opaque data from the CPS data returned by Get

        @cps_data - CPS data returned by Get
        """
        return utl.extract_cps_attr(self, self.cps_data, 'data')

    def extract_attr(self, attr_name):
        """
        Get value for any attribute from the CPS data returned by Get
        """
        return utl.extract_cps_attr(self, self.cps_data, attr_name)
