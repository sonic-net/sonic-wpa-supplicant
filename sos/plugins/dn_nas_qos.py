#!/usr/bin/python
# -*- coding: utf-8 -*-

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
from sos.plugins import Plugin, DebianPlugin
import os

class DN_nas_qos(Plugin, DebianPlugin):
    """ Collects nas-qos debugging information
    """

    plugin_name = os.path.splitext(os.path.basename(__file__))[0]
    profiles = ('networking', 'dn')

    def setup(self):
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/meter")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/wred-profile")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/queue")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/queue-stat")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/scheduler-profile")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/scheduler-group")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/dot1p-to-tc-color-map")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/dscp-to-tc-color-map")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/tc-to-queue-map")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/tc-color-to-dot1p-map")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/tc-color-to-dscp-map")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/port-ingress")
        self.add_cmd_output("/opt/dell/os10/bin/cps_get_oid.py base-qos/port-egress")
