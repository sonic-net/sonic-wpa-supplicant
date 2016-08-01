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


def wred_profile_create_example():
    attr_list = {
        'green-enable': 1,
        'green-min-threshold': 20,
        'green-max-threshold': 80,
        'green-drop-probability': 10,
        'yellow-enable': 1,
        'yellow-min-threshold': 40,
        'yellow-max-threshold': 60,
        'yellow-drop-probability': 30,
        'weight': 8,
        'ecn-enable': 1,
        'npu-id-list': [0],
    }

    wred_obj = nas_qos.WredCPSObj(map_of_attr=attr_list)
    upd = ('create', wred_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "WRED profile creation failed"
        return None

    print 'Return = ', ret_cps_data
    wred_obj = nas_qos.WredCPSObj(cps_data=ret_cps_data[0])
    wred_id = wred_obj.extract_attr('id')
    print "Successfully installed WRED id = ", wred_id

    return wred_id


def wred_profile_get_example(wred_id):
    return_data_list = []

    wred_obj = nas_qos.WredCPSObj(wred_id=wred_id)
    ret = cps.get([wred_obj.data()], return_data_list)

    if ret:
        print '#### WRED Profile Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.WredCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'


def wred_profile_modify_attrs(wred_id, mod_attr_list):
    wred_obj = nas_qos.WredCPSObj(wred_id=wred_id)
    for attr in mod_attr_list:
        wred_obj.set_attr(attr[0], attr[1])

    upd = ('set', wred_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "WRED profile modification failed"
        return None

    print 'Return = ', ret_cps_data
    wred_obj = nas_qos.WredCPSObj(cps_data=ret_cps_data[0])
    wred_id = wred_obj.extract_attr('id')
    print "Successfully modified WRED id = ", wred_id

    return wred_id


def wred_profile_modify_example_1(wred_id):
    mod_attrs = [
        ('yellow-min-threshold', 50),
        ('yellow-max-threshold', 80),
        ('red-min-threshold', 70),
        ('red-max-threshold', 100)
    ]
    return wred_profile_modify_attrs(wred_id, mod_attrs)


def wred_profile_modify_example_2(wred_id):
    mod_attrs = [
        ('yellow-enable', 1),
        ('red-enable', 1)
    ]
    return wred_profile_modify_attrs(wred_id, mod_attrs)


def wred_profile_delete_example(wred_id):
    wred_obj = nas_qos.WredCPSObj(wred_id=wred_id)
    upd = ('delete', wred_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "WRED profile delete failed"
        return None

    print 'Return = ', ret_cps_data
    print "Successfully deleted WRED id = ", wred_id

if __name__ == '__main__':
    wred_id = wred_profile_create_example()
    if wred_id is None:
        exit()
    wred_profile_get_example(wred_id)
    wred_id = wred_profile_modify_example_1(wred_id)
    if wred_id is None:
        exit()
    wred_id = wred_profile_modify_example_2(wred_id)
    if wred_id is None:
        exit()
    wred_profile_get_example(wred_id)
    wred_profile_delete_example(wred_id)
