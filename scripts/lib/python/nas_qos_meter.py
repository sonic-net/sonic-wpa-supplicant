#!/usr/bin/python
#
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#

import cps_utils
import cps
import nas_qos_utils as utl


class MeterCPSObj:

    """
    Utility class to build a CPS Object based on the QoS Meter Yang model
    with attributes populated from the values passed as inputs.
    The resulting CPS Object representation, obtained by calling data() method,
    can be plugged into a CPS Transaction (by adding create,delete or set op)
    or directly into a CPS Get request.

    An instance of this class can also act as a wrapper over the CPS object
    dictionary returned by CPS Create or Get on Meter Yang model and
    provides methods to extract various attributes from it.
    """

    yang_name = 'meter'

    type_map = {
        'switch-id': ('leaf', 'uint32_t'),
        'id': ('leaf', 'uint64_t'),
        'type': ('leaf', 'enum', 'base-qos:meter-type:'),
        'mode': ('leaf', 'enum', 'base-qos:meter-mode:'),
        'color-source': ('leaf', 'enum', 'base-qos:meter-color-source:'),
        'committed-burst': ('leaf', 'uint64_t'),
        'committed-rate': ('leaf', 'uint64_t'),
        'peak-burst': ('leaf', 'uint64_t'),
        'peak-rate': ('leaf', 'uint64_t'),
        'green-packet-action': ('leaf', 'enum', 'base-qos:policer-action:'),
        'yellow-packet-action': ('leaf', 'enum', 'base-qos:policer-action:'),
        'red-packet-action': ('leaf', 'enum', 'base-qos:policer-action:'),
    }

    @classmethod
    def get_type_map(cls):
        return cls.type_map

    def __init__(
        self, meter_type=None, cir=None, cbs=None, pbs=None, pir=None,
            meter_id=None, switch_id=0, cps_data=None):

        if cps_data is not None:
            self.cps_data = cps_data
            return

        self.cps_data = None
        self.cps_obj_wr = utl.CPSObjWrp(self.yang_name, self.get_type_map())
        self.mode_detected = True

        if meter_id is not None:
            # Detect Meter Mode only for the Create case
            self.mode_detected = False
            self.cps_obj_wr.add_leaf_attr('id', meter_id)

        self.cps_obj_wr.add_leaf_attr('switch-id', switch_id)

        if meter_type is not None:
            self.cps_obj_wr.add_leaf_attr('type', meter_type)
        if cir is not None:
            self.cps_obj_wr.add_leaf_attr('committed-rate', cir)
        if cbs is not None:
            self.cps_obj_wr.add_leaf_attr('committed-burst', cbs)
        if pbs is not None:
            self.cps_obj_wr.add_leaf_attr('peak-burst', pbs)
        if pir is not None:
            self.cps_obj_wr.add_leaf_attr('peak-rate', pir)

    def attrs(self):
        return self.cps_obj_wr.get_attrs()

    def detect_meter_mode(self):
        if not self.mode_detected:
            return
        if 'committed-rate' in self.attrs() and 'committed-burst' in self.attrs():
            if 'peak-burst' in self.attrs():
                if 'peak-rate' in self.attrs():
                    self.cps_obj_wr.add_leaf_attr('mode', 'Tr_TCM')
                    return
                self.cps_obj_wr.add_leaf_attr('mode', 'Sr_TCM')
                return
            self.cps_obj_wr.add_leaf_attr('mode', 'Sr_TWO_COLOR')
            return

    def set_attr(self, attr_name, attr_val):
        if attr_name == 'mode':
            self.mode_detected = False
        self.cps_obj_wr.add_leaf_attr(attr_name, attr_val)

    def data(self):
        if self.cps_data is not None:
            return self.cps_data

        self.detect_meter_mode()
        return self.cps_obj_wr.get()

    def extract_id(self):
        """
        Get Meter ID from the CPS data returned by Create or Get
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
