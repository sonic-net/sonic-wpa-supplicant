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
import nas_qos
import nas_qos_buffer_pool_example
import nas_qos_buffer_profile_example

def priority_group_get_example(port_id, local_id):
    return_data_list = []

    priority_group_obj = nas_qos.PriorityGroupCPSObj(
        local_id=local_id,
        port_id=port_id)
    ret = cps.get([priority_group_obj.data()], return_data_list)

    if ret:
        print '#### PG Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.PriorityGroupCPSObj(
                port_id=port_id,
                local_id=local_id,
                cps_data=cps_ret_data)
            m.print_obj()
    else:
        print "Error in Get"


def priority_group_modify_buffer_profile_example (port_id, local_id, buffer_profile_id):
    m = nas_qos.PriorityGroupCPSObj (local_id=local_id, port_id=port_id)
    m.set_attr ('buffer-profile-id', buffer_profile_id)

    upd = ('set', m.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "PG modify Failed"
        return None

    print 'Return = ', ret_cps_data
    m = nas_qos.PriorityGroupCPSObj(None, None, cps_data=ret_cps_data[0])
    port_id = m.extract_attr('port-id')
    local_id = m.extract_attr('local-id')
    print "Successfully modified PG of Port %d local-id %d" % (port_id, local_id)

    return ret_cps_data

def priority_group_stat_get_example (port_id, local_id):
    return_data_list = []

    priority_group_stat_obj = nas_qos.PriorityGroupStatCPSObj (port_id=port_id, local_id=local_id)
    ret = cps.get ([priority_group_stat_obj.data()], return_data_list)

    if ret == True:
        print '#### PriorityGroup Stat Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.PriorityGroupStatCPSObj (cps_data = cps_ret_data)
            m.print_obj ()
    else:
        print "Error in Get"


def buffer_pool_stat_get_example (id):
    return_data_list = []

    buffer_pool_stat_obj = nas_qos.BufferPoolStatCPSObj (id)
    ret = cps.get ([buffer_pool_stat_obj.data()], return_data_list)

    if ret == True:
        print '#### BufferPool Stat Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.BufferPoolStatCPSObj (cps_data = cps_ret_data)
            m.print_obj ()
    else:
        print "Error in Get"



if __name__ == '__main__':
    port_id = 17

    print '### Show all priority_groups of port %d ###' % port_id
    priority_group_get_example(port_id, None)

    buffer_pool_id = nas_qos_buffer_pool_example.buffer_pool_create_example('INGRESS')
    if buffer_pool_id is None:
        exit()

    buffer_profile_id = nas_qos_buffer_profile_example.buffer_profile_create_example(buffer_pool_id)
    if buffer_profile_id is None:
        exit()

    local_id = 1
    priority_group_get_example(port_id, local_id)
    priority_group_id = priority_group_modify_buffer_profile_example(port_id, local_id, buffer_profile_id)
    if priority_group_id is None:
        exit()

    priority_group_get_example(port_id, local_id)

    # stats get
    print '### Show buffer_pool stat of buffer_pool_id %d ###' % buffer_pool_id
    buffer_pool_stat_get_example (buffer_pool_id)

    # stats get
    print '### Show priority_group stat of port %d local_id %d ###' % (port_id, local_id)
    priority_group_stat_get_example (port_id, local_id )


    # reset
    priority_group_id = priority_group_modify_buffer_profile_example(port_id, local_id, 0)
    if priority_group_id is None:
        exit()

    nas_qos_buffer_profile_example.buffer_profile_delete_example(buffer_profile_id)
    nas_qos_buffer_pool_example.buffer_pool_delete_example(buffer_pool_id)

