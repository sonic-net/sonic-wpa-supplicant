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

def buffer_profile_create_example(pool_id, pool_type = 'INGRESS'):
    if (pool_type == 'EGRESS'):
        attr_list = {
            'pool-id': pool_id,
            'buffer-size': 2000,
            'shared-static-threshold': 500,
        }
    else :
        attr_list = {
            'pool-id': pool_id,
            'buffer-size': 2000,
            'shared-static-threshold': 500,
            'xoff-threshold': 1000,
            'xon-threshold':  5000,
        }

    buffer_profile_obj = nas_qos.BufferProfileCPSObj(map_of_attr=attr_list)
    upd = ('create', buffer_profile_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "buffer profile creation failed"
        return None

    print 'Return = ', ret_cps_data
    buffer_profile_obj = nas_qos.BufferProfileCPSObj(cps_data=ret_cps_data[0])
    buffer_profile_id = buffer_profile_obj.extract_attr('id')
    print "Successfully installed buffer profile id = ", buffer_profile_id

    return buffer_profile_id


def buffer_profile_get_example(buffer_profile_id):
    return_data_list = []

    buffer_profile_obj = nas_qos.BufferProfileCPSObj(buffer_profile_id=buffer_profile_id)
    ret = cps.get([buffer_profile_obj.data()], return_data_list)

    if ret:
        print '#### buffer profile Profile Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.BufferProfileCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'


def buffer_profile_modify_attrs(buffer_profile_id, mod_attr_list):
    buffer_profile_obj = nas_qos.BufferProfileCPSObj(buffer_profile_id=buffer_profile_id)
    for attr in mod_attr_list:
        buffer_profile_obj.set_attr(attr[0], attr[1])

    upd = ('set', buffer_profile_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "buffer profile modification failed"
        return None

    print 'Return = ', ret_cps_data
    buffer_profile_obj = nas_qos.BufferProfileCPSObj(cps_data=ret_cps_data[0])
    buffer_profile_id = buffer_profile_obj.extract_attr('id')
    print "Successfully modified buffer profile id = ", buffer_profile_id

    return buffer_profile_id


def buffer_profile_modify_example(buffer_profile_id):
    mod_attrs = [
        ('xoff-threshold', 800),
        ('xon-threshold', 4000)
    ]
    return buffer_profile_modify_attrs(buffer_profile_id, mod_attrs)


def buffer_profile_delete_example(buffer_profile_id):
    buffer_profile_obj = nas_qos.BufferProfileCPSObj(buffer_profile_id=buffer_profile_id)
    upd = ('delete', buffer_profile_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "buffer profile delete failed"
        return None

    print 'Return = ', ret_cps_data
    print "Successfully deleted buffer profile id = ", buffer_profile_id

if __name__ == '__main__':
    buffer_pool_id = nas_qos_buffer_pool_example.buffer_pool_create_example('INGRESS')
    if buffer_pool_id is None:
        exit()

    buffer_profile_id = buffer_profile_create_example(buffer_pool_id)
    if buffer_profile_id is None:
        exit()

    buffer_profile_get_example(buffer_profile_id)
    buffer_profile_id = buffer_profile_modify_example(buffer_profile_id)
    if buffer_profile_id is None:
        exit()
    buffer_profile_get_example(buffer_profile_id)

    buffer_profile_delete_example(buffer_profile_id)
    nas_qos_buffer_pool_example.buffer_pool_delete_example(buffer_pool_id)

