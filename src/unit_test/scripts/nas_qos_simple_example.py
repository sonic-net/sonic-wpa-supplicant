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

'''
   This simply example illustrates:
     1. how to configure a port to control ingress broadcast storm
        using a policer (a.k.a. meter) profile
     2. how to configure a port to rate-limit the egress traffic
        using a scheduler profile
'''

import nas_qos
import cps
import cps_utils
import sys
import nas_qos_utils as utl


def config_qos_port_ingress_example(port_name):
    print '\nconfigure qos port-ingress for port %s\n' % port_name

    print 'Start to create storm control policer profiles'
    bcast_storm_control_id = nas_qos.create_storm_control_policer(300)

    print 'Setup port ingress profile with policers'
    nas_qos.set_port_ingress_bcast_storm_control(port_name, bcast_storm_control_id)

    print 'Reset policer in port ingress profile'
    nas_qos.set_port_ingress_bcast_storm_control(port_name, 0)

    print 'Start to delete storm-control policer profiles'
    nas_qos.delete_policer(bcast_storm_control_id)

    return True


def config_qos_port_egress_example(port_name):
    print '\nConfigure qos port-egress for port %s\n' % port_name

    print 'Start to create scheduler profile'
    sched_id = nas_qos.scheduler_profile_create(
        'WRR', 50, 10000, 200, 50000, 200)

    print 'Setup port egress porfile with scheduler'
    nas_qos.set_port_egress_scheduler_id(port_name, sched_id)

    print 'Reset scheduler in port egress profile'
    nas_qos.set_port_egress_scheduler_id(port_name, 0)

    print 'Start to delete scheduler profile'
    nas_qos.scheduler_profile_delete(sched_id)

    return True

if __name__ == '__main__':
    port_name = 'e00-1'
    if config_qos_port_ingress_example(port_name) == False:
        exit()
    if config_qos_port_egress_example(port_name) == False:
        exit()
