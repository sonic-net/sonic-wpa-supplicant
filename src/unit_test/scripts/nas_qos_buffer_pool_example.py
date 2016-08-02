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


def buffer_pool_create_example(pool_type):
    attr_list = {
        'pool-type': pool_type,
        'size': 20000,
        'threshold-mode': 'STATIC',
    }

    buffer_pool_obj = nas_qos.BufferPoolCPSObj(map_of_attr=attr_list)
    upd = ('create', buffer_pool_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "buffer pool creation failed"
        return None

    print 'Return = ', ret_cps_data
    buffer_pool_obj = nas_qos.BufferPoolCPSObj(cps_data=ret_cps_data[0])
    buffer_pool_id = buffer_pool_obj.extract_attr('id')
    print "Successfully installed buffer pool id = ", buffer_pool_id

    return buffer_pool_id


def buffer_pool_get_example(buffer_pool_id):
    return_data_list = []

    buffer_pool_obj = nas_qos.BufferPoolCPSObj(buffer_pool_id=buffer_pool_id)
    ret = cps.get([buffer_pool_obj.data()], return_data_list)

    if ret:
        print '#### buffer pool Profile Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.BufferPoolCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'


def buffer_pool_modify_attrs(buffer_pool_id, mod_attr_list):
    buffer_pool_obj = nas_qos.BufferPoolCPSObj(buffer_pool_id=buffer_pool_id)
    for attr in mod_attr_list:
        buffer_pool_obj.set_attr(attr[0], attr[1])

    upd = ('set', buffer_pool_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "buffer pool modification failed"
        return None

    print 'Return = ', ret_cps_data
    buffer_pool_obj = nas_qos.BufferPoolCPSObj(cps_data=ret_cps_data[0])
    buffer_pool_id = buffer_pool_obj.extract_attr('id')
    print "Successfully modified buffer pool id = ", buffer_pool_id

    return buffer_pool_id


def buffer_pool_modify_example(buffer_pool_id):
    mod_attrs = [
        ('size', 30000),
    ]
    return buffer_pool_modify_attrs(buffer_pool_id, mod_attrs)


def buffer_pool_delete_example(buffer_pool_id):
    buffer_pool_obj = nas_qos.BufferPoolCPSObj(buffer_pool_id=buffer_pool_id)
    upd = ('delete', buffer_pool_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "buffer pool delete failed"
        return None

    print 'Return = ', ret_cps_data
    print "Successfully deleted buffer pool id = ", buffer_pool_id

if __name__ == '__main__':
    buffer_pool_id = buffer_pool_create_example('INGRESS')
    if buffer_pool_id is None:
        exit()
    buffer_pool_get_example(buffer_pool_id)
    buffer_pool_id = buffer_pool_modify_example(buffer_pool_id)
    if buffer_pool_id is None:
        exit()
    buffer_pool_get_example(buffer_pool_id)
    buffer_pool_delete_example(buffer_pool_id)
