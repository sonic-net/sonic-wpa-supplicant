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

'''
   This simply example illustrates:
     1. how to configure a port to control ingress broadcast storm
        using a policer (a.k.a. meter) profile
     2. how to configure a port to rate-limit the egress traffic
        using a scheduler profile
'''

import cps
import cps_utils
import sys


e_meter_type = {'PACKET': 1, 'BYTE': 2}
e_meter_mode = {'Sr_TCM': 1, 'Tr_TCM': 2, 'Sr_TWO_COLOR':3, 'STORM_CONTROL':4}
e_scheduling_type = {'SP': 1, 'WRR':2, 'WDRR': 3}
e_policer_action = {'FORWARD': 1, 'DROP': 2}

type_map = {
    'base-qos/meter/peak-rate': 'uint64_t',
    'base-qos/meter/id': 'uint64_t',
}

for key,val in type_map.items():
    cps_utils.cps_attr_types_map.add_type(key, val)

# Sample interface
ifindex = 16

print '\nconfigure qos port-ingress for port %s\n' % ifindex

print 'Start to create storm control policer profiles'
meter_obj = cps_utils.CPSObject(module='base-qos/meter',
                                data = {'type': e_meter_type['PACKET'],
                                            'peak-rate': 300,
                                        'mode': e_meter_mode['STORM_CONTROL']})
upd = ('create', meter_obj.get())
r = cps_utils.CPSTransaction([upd]).commit()
if not r:
    raise RuntimeError ("Error creating "+ 'base-qos/meter')
ret = cps_utils.CPSObject (module='base-qos/meter', obj=r[0]['change'])
meter_id = ret.get_attr_data ('id')

print 'Setup port ingress profile with policers'
port_ing_obj = cps_utils.CPSObject(module = 'base-qos/port-ingress',
                                   data = {'port-id': ifindex,
                                           'broadcast_storm_control': meter_id})
upd = ('set', port_ing_obj.get())
r = cps_utils.CPSTransaction([upd]).commit()
if not r:
    raise RuntimeError ("Error setting "+ 'base-qos/port-egress')



print '\nConfigure qos port-egress for port %s\n' % ifindex

print 'Start to create scheduler profile'
attr_list = {
    'algorithm': e_scheduling_type['WRR'],
    'weight': 50,
    'meter-type': e_meter_type['PACKET'],
    'min-rate': 10000,
    'min-burst': 200,
    'max-rate': 50000,
    'max-burst': 200,
    'npu-id-list': [0],
}
sched_obj = cps_utils.CPSObject(module='base-qos/scheduler-profile',
                                data = attr_list)

upd = ('create', sched_obj.get())
r = cps_utils.CPSTransaction([upd]).commit()
if not r:
    raise RuntimeError ("Error creating "+ 'base-qos/scheduler-profile')
ret = cps_utils.CPSObject (module='base-qos/scheduler-profile', obj=r[0]['change'])
sched_id = ret.get_attr_data ('id')

print 'Setup port egress porfile with scheduler'
port_eg_obj = cps_utils.CPSObject(module = 'base-qos/port-egress',
                                  data = {'port-id': ifindex,
                                              'scheduler-profile-id': sched_id})
upd = ('set', port_eg_obj.get())
r = cps_utils.CPSTransaction([upd]).commit()
if not r:
    raise RuntimeError ("Error setting "+ 'base-qos/port-egress')


