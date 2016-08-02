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


def policer_srtcm_create_example():

    m = nas_qos.MeterCPSObj(
        meter_type='BYTE',
        cir=300000,
        cbs=800000,
        pbs=900000)
    upd = ('create', m.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "Meter install Failed"
        return None

    print 'Return = ', ret_cps_data
    m = nas_qos.MeterCPSObj(cps_data=ret_cps_data[0])
    meter_id = m.extract_id()
    print "Successfully installed Meter Id = ", meter_id
    return meter_id


def policer_get_example(meter_id):
    return_data_list = []

    filt_obj = nas_qos.MeterCPSObj(meter_id=meter_id)
    ret = cps.get([filt_obj.data()], return_data_list)

    if ret:
        print '#### Meter Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.MeterCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print "Error in Get"


def policer_modify_example(meter_id):
    m = nas_qos.MeterCPSObj(meter_id=meter_id, cir=400000)
    m.set_attr('green-packet-action', 'DROP')
    m.set_attr('yellow-packet-action', 'DROP')

    upd = ('set', m.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "Meter modify Failed"
        return None

    print 'Return = ', ret_cps_data
    m = nas_qos.MeterCPSObj(cps_data=ret_cps_data[0])
    meter_id = m.extract_id()
    print "Successfully modified Meter Id = ", meter_id
    return meter_id


def policer_delete_example(meter_id):
    m = nas_qos.MeterCPSObj(meter_id=meter_id)
    upd = ('delete', m.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "Meter delete Failed"
        return None

    print 'Return = ', ret_cps_data
    print "Successfully deleted Meter Id = ", meter_id


def policer_trtcm_create_example():

    m = nas_qos.MeterCPSObj(meter_type='BYTE', cir=300000, cbs=800000,
                            pbs=900000, pir=500000)
    upd = ('create', m.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print "Meter install Failed"
        return None

    print 'Return = ', ret_cps_data
    m = nas_qos.MeterCPSObj(cps_data=ret_cps_data[0])
    meter_id = m.extract_id()
    print "Successfully installed Meter Id = ", meter_id
    return meter_id

if __name__ == '__main__':
    meter_id = policer_srtcm_create_example()
    if meter_id is None:
        exit()
    policer_get_example(meter_id)
    meter_id = policer_modify_example(meter_id)
    if meter_id is None:
        exit()
    policer_get_example(meter_id)
    policer_delete_example(meter_id)

    meter_id = policer_trtcm_create_example()
    if meter_id is None:
        exit()
    policer_get_example(meter_id)
    policer_delete_example(meter_id)
