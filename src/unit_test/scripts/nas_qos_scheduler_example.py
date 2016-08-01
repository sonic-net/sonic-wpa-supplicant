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


def scheduler_profile_create_example():
    attr_list = {
        'algorithm': 'WRR',
        'weight': 20,
        'meter-type': 'PACKET',
        'min-rate': 100,
        'min-burst': 100,
        'max-rate': 500,
        'max-burst': 100,
        'npu-id-list': [0],
    }

    sched_obj = nas_qos.SchedulerCPSObj(map_of_attr=attr_list)
    upd = ('create', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print 'Scheduler profile cration failed'
        return None

    print 'Return = ', ret_cps_data
    sched_obj = nas_qos.SchedulerCPSObj(cps_data=ret_cps_data[0])
    sched_id = sched_obj.extract_id()
    print 'Successfully installed Scheduler profile id = ', sched_id

    return sched_id


def scheduler_profile_get_example(sched_id):
    return_data_list = []

    sched_obj = nas_qos.SchedulerCPSObj(sched_id=sched_id)
    ret = cps.get([sched_obj.data()], return_data_list)

    if ret:
        print '#### Scheduler Profile Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.SchedulerCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'


def scheduler_profile_modify_attrs(sched_id, mod_attr_list):
    sched_obj = nas_qos.SchedulerCPSObj(sched_id=sched_id)
    for attr in mod_attr_list:
        sched_obj.set_attr(attr[0], attr[1])

    upd = ('set', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        print 'Scheduler profile modification failed'
        return None

    print 'Return = ', ret_cps_data
    sched_obj = nas_qos.SchedulerCPSObj(cps_data=ret_cps_data[0])
    sched_id = sched_obj.extract_id()
    print 'Successfully modified Scheduler id = ', sched_id

    return sched_id


def scheduler_profile_modify_example(sched_id):
    mod_attrs = [
        ('max-rate', 800),
        ('max-burst', 200),
    ]
    return scheduler_profile_modify_attrs(sched_id, mod_attrs)


def scheduler_profile_delete_example(sched_id):
    sched_obj = nas_qos.SchedulerCPSObj(sched_id=sched_id)
    upd = ('delete', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print 'Scheduler profile delete failed'
        return None

    print 'Return = ', ret_cps_data
    print 'Successfully delted Scheduler id = ', sched_id

if __name__ == '__main__':
    sched_id = scheduler_profile_create_example()
    if sched_id is None:
        exit()
    scheduler_profile_get_example(sched_id)
    sched_id = scheduler_profile_modify_example(sched_id)
    if sched_id is None:
        exit()
    scheduler_profile_get_example(sched_id)
    scheduler_profile_delete_example(sched_id)
