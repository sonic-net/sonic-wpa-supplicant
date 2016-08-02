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
import nas_qos

def queue_stat_get_example (port_id, queue_type, queue_number ):
    return_data_list = []

    queue_stat_obj = nas_qos.QueueStatCPSObj (queue_type=queue_type, queue_number=queue_number, port_id=port_id)
    ret = cps.get ([queue_stat_obj.data()], return_data_list)

    if ret == True:
        print '#### Queue Stat Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.QueueStatCPSObj (cps_data = cps_ret_data)
            m.print_obj ()
    else:
        print "Error in Get"


if __name__ == '__main__':
    port_id = 34

    print '### Show unicast queue 1 of port %d ###' % port_id
    queue_stat_get_example (port_id, 'UCAST', 1 )

    print '### Show unicast queue 2 of port %d ###' % port_id
    queue_stat_get_example (port_id, 'UCAST', 2 )

    print '### Show mcast queue 1 of port %d ###' % port_id
    queue_stat_get_example (port_id, 'MULTICAST', 1 )

    print '### Show mcast queue 2 of port %d ###' % port_id
    queue_stat_get_example (port_id, 'MULTICAST', 2 )


