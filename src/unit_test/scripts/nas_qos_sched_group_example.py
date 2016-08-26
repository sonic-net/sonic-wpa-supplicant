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
import sys
import nas_qos
import nas_qos_utils as utl


def get_port_queue_id_list(port_name, queue_type):
    cps_data_list = []
    ret_data_list = []

    ifindex = utl.ifname_to_ifindex(port_name)
    if ifindex is None:
        print 'Failed to get ifindex for port ', port_name
        return None

    if queue_type == 'ALL':
        queue_type = None

    queue_obj = nas_qos.QueueCPSObj(
        port_id=ifindex,
        queue_type=queue_type,
        queue_number=None)
    ret = cps.get([queue_obj.data()], cps_data_list)

    if ret == False:
        print 'Failed to get queue list'
        return None

    print '-' * 36
    print '%-16s %-10s %s' % ('id', 'type', 'number')
    print '-' * 36
    for cps_data in cps_data_list:
        m = nas_qos.QueueCPSObj(None, None, None, cps_data=cps_data)
        queue_id = m.extract_id()
        type_val = m.extract_attr('type')
        local_num = m.extract_attr('queue-number')
        if queue_type is None or queue_type == type_val:
            print '%-16x %-10s %s' % (queue_id, type_val, local_num)
            ret_data_list.append(queue_id)

    return ret_data_list


def scheduler_group_create_example(port_name, level):
    sg_obj = nas_qos.SchedGroupCPSObj(port_name, level)
    upd = ('create', sg_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print 'Scheduler Group creation failed'
        return None

    sg_obj = nas_qos.SchedGroupCPSObj(cps_data=ret_cps_data[0])
    sg_id = sg_obj.extract_id()
    print 'Successfully installed Scheduler Group id = ', sg_id

    return sg_id


def scheduler_group_get_example(sg_id=None, port_name=None, level=None):
    return_data_list = []
    sg_obj = nas_qos.SchedGroupCPSObj(sg_id=sg_id, port_name=port_name,
                                      level=level)
    ret = cps.get([sg_obj.data()], return_data_list)
    if ret:
        print '#### Scheduler Group Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.SchedGroupCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'


def scheduler_group_delete_example(sg_id):
    sg_obj = nas_qos.SchedGroupCPSObj(sg_id=sg_id)
    upd = ('delete', sg_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print 'Scheduler Group delete failed'
        return None

    print 'Successfully deleted Scheduler Group id = ', sg_id
    return ret_cps_data


def scheduler_group_cleanup_example(port_name, level):
    return_data_list = []
    sg_obj = nas_qos.SchedGroupCPSObj(port_name=port_name, level=level)
    ret = cps.get([sg_obj.data()], return_data_list)
    if ret != True:
        print 'Error in get'
        return False
    for cps_ret_data in return_data_list:
        m = nas_qos.SchedGroupCPSObj(cps_data=cps_ret_data)
        sg_id = m.extract_id()
        sched_id = m.extract_attr('scheduler-profile-id')
        if level != 0 and sched_id is not None and sched_id != 0:
            sched_id = 0
        else:
            sched_id = None
        chld_cnt = m.extract_attr('child_count')
        if chld_cnt > 0:
            chld_list = [0]
        else:
            chld_list = []
        ret_sg_id = scheduler_group_modify_example(sg_id, sched_id, chld_list)
        if ret_sg_id is None:
            print 'Falied to modify scheduler-group %d' % sg_id
            return False
    print 'Successfully cleaned up scheduler-groups of port %s level %d' % (
        port_name, level)
    return True


def scheduler_group_delete_all_example(port_name):
    if scheduler_group_cleanup_example(port_name, 2) == False:
        return False
    if scheduler_group_cleanup_example(port_name, 1) == False:
        return False
    if scheduler_group_cleanup_example(port_name, 0) == False:
        return False
    return_data_list = []
    sg_obj = nas_qos.SchedGroupCPSObj(port_name=port_name)
    ret = cps.get([sg_obj.data()], return_data_list)
    if ret != True:
        print 'Failed to get scheduler-groups of port %s' % port_name
        return False
    for ret_cps_data in return_data_list:
        ret_sg_obj = nas_qos.SchedGroupCPSObj(cps_data=ret_cps_data)
        sg_id = ret_sg_obj.extract_id()
        print 'Delete scheduler-group %d' % sg_id
        ret = scheduler_group_delete_example(sg_id)
        if ret is None:
            print 'Failed to delete scheduler-group'
            return False
    print 'Successfully deleted all scheduler-groups of port %s' % port_name
    return True


def scheduler_group_modify_example(sg_id, sched_id=None, chld_id_list=[]):
    sg_obj = nas_qos.SchedGroupCPSObj(sg_id=sg_id, sched_id=sched_id,
                                      chld_id_list=chld_id_list)
    upd = ('set', sg_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        print 'Scheduler Group modification failed'
        return None
    m = nas_qos.SchedGroupCPSObj(cps_data=ret_cps_data[0])
    sg_id = m.extract_id()
    print 'Successfully modified Scheduler Group id = ', sg_id
    return sg_id


def scheduler_profile_create_example(
        algo, weight, min_rate, min_burst, max_rate, max_burst):
    attr_list = {
        'algorithm': algo,
        'weight': weight,
        'meter-type': 'PACKET',
        'min-rate': min_rate,
        'min-burst': min_burst,
        'max-rate': min_rate,
        'max-burst': max_burst,
        'npu-id-list': [0],
    }
    sched_obj = nas_qos.SchedulerCPSObj(map_of_attr=attr_list)
    upd = ('create', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        print 'Scheduler profile creation failed'
        return None

    sched_obj = nas_qos.SchedulerCPSObj(cps_data=ret_cps_data[0])
    sched_id = sched_obj.extract_id()
    print 'Successfully installed Scheduler profile id = ', sched_id

    return sched_id


def scheduler_profile_get_example(port_name, level=None):
    return_data_list = []

    sched_obj = nas_qos.SchedulerCPSObj(port_name=port_name, level=level)
    ret = cps.get([sched_obj.data()], return_data_list)

    if ret:
        print '#### Scheduler Profile Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.SchedulerCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'


def scheduler_profile_delete_example(sched_id):
    sched_obj = nas_qos.SchedulerCPSObj(sched_id=sched_id)
    upd = ('delete', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print 'Scheduler profile delete failed'
        return None

    print 'Successfully deleted Scheduler id = ', sched_id

    return ret_cps_data


def get_first_phy_port():
    ret_data_list = utl.iflist_get(if_type=1)
    if not ret_data_list:
        ret_data_list = utl.iflist_get(if_type=2)
    if not ret_data_list:
        return None
    name_list = []
    for ret_data in ret_data_list:
        cps_obj = cps_utils.CPSObject(obj=ret_data)
        port_name = cps_obj.get_attr_data('name')
        name_list.append(port_name)
    name_list.sort()
    return name_list[0]

if __name__ == '__main__':
    if len(sys.argv) >= 2:
        port_name = sys.argv[1]
    else:
        port_name = get_first_phy_port()
    if port_name is None:
        print 'Could not find front port'
        exit()
    print 'Using port %s' % port_name

    # Delete all existing scheduler-groups
    ret = scheduler_group_delete_all_example(port_name)
    if ret == False:
        exit()

    # Create scheduler-group
    sg_id_root = scheduler_group_create_example(port_name, 0)
    if sg_id_root is None:
        exit()
    sg_id_l0 = scheduler_group_create_example(port_name, 1)
    if sg_id_l0 is None:
        exit()
    sg_id_l1_uc = scheduler_group_create_example(port_name, 2)
    if sg_id_l1_uc is None:
        exit()
    sg_id_l1_mc = scheduler_group_create_example(port_name, 2)
    if sg_id_l1_mc is None:
        exit()

    # Create scheduler profile
    sched_id_l0 = scheduler_profile_create_example(
        'WRR', 50, 100, 100, 500, 100)
    if sched_id_l0 is None:
        exit()
    sched_id_l1_uc = scheduler_profile_create_example(
        'WRR', 30, 50, 100, 200, 100)
    if sched_id_l1_uc is None:
        exit()
    sched_id_l1_mc = scheduler_profile_create_example(
        'WDRR', 30, 50, 100, 200, 100)
    if sched_id_l1_mc is None:
        exit()

    chld_list = [sg_id_l0]
    sg_id = scheduler_group_modify_example(sg_id_root, None, chld_list)
    if sg_id is None:
        exit()
    chld_list = [sg_id_l1_uc, sg_id_l1_mc]
    sg_id = scheduler_group_modify_example(sg_id_l0, sched_id_l0, chld_list)
    if sg_id is None:
        exit()

    # Add unicast/multicast queues to L1 scheduler-group
    queue_id_list = get_port_queue_id_list(port_name, 'UCAST')
    if queue_id_list is None:
        print 'Failed to get unicast queue list'
        exit()
    sg_id = scheduler_group_modify_example(
        sg_id_l1_uc,
        sched_id_l1_uc,
        queue_id_list)
    if sg_id is None:
        exit()

    queue_id_list = get_port_queue_id_list(port_name, 'MULTICAST')
    if queue_id_list is None:
        print 'Failed to get multicast queue list'
        exit()
    sg_id = scheduler_group_modify_example(
        sg_id_l1_mc,
        sched_id_l1_mc,
        queue_id_list)
    if sg_id is None:
        exit()

    scheduler_group_get_example(port_name=port_name)
