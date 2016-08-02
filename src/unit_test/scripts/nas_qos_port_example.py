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

import nas_qos
import cps
import cps_utils
import sys
import nas_qos_utils as utl


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


def create_map_example(map_name, entries):
    map_obj = nas_qos.MapCPSObjs(map_name, entry_list=entries)
    if map_obj.commit() == False:
        print 'Failed to create map %s' % map_name
        return None

    print 'Successfully created map %s with map_id %d' % (
        map_name, map_obj.get_map_id())
    return map_obj.get_map_id()


def delete_map_example(map_name, map_id):
    map_obj = nas_qos.MapCPSObjs(map_name, map_id=map_id)
    if map_obj.delete() == False:
        print 'Falied to delete map %s' % map_name
        return False

    print 'Successfully deleted map %s with map_id %d' % (
        map_name, map_id)
    return True


def get_map_example(map_name, map_id):
    map_obj = nas_qos.MapCPSObjs(map_name, map_id=map_id)
    ret_data_list = []
    ret = cps.get([map_obj.data_map()], ret_data_list)

    if ret != True:
        print 'Error in get'
        return False

    print '#### Map %s Profile ####' % map_name
    for ret_data in ret_data_list:
        map_obj = nas_qos.MapCPSObjs(map_name, cps_data=ret_data)
        map_obj.print_obj()


def create_storm_control_policer_example(peak_rate):
    meter_obj = nas_qos.MeterCPSObj(meter_type='PACKET', pir=peak_rate)
    meter_obj.set_attr('mode', 'STORM_CONTROL')
    upd = ('create', meter_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        print 'Policer creation failed'
        return None

    meter_obj = nas_qos.MeterCPSObj(cps_data=ret_cps_data[0])
    meter_obj.print_obj()
    meter_id = meter_obj.extract_id()
    print 'Successfully created Policer Id = %d' % meter_id
    return meter_id


def delete_policer_example(meter_id):
    meter_obj = nas_qos.MeterCPSObj(meter_id=meter_id)
    upd = ('delete', meter_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        print 'Policer delete failed'
        return None

    print 'Successfully deleted Policer Id = %d' % meter_id


def test_qos_port_ingress(port_name):
    print 'Test qos port-ingress for port %s' % port_name
    print 'Start to create port-ingress map profiles'
    tc_queue_id = None
    if not port_name.find('npu') == 0:
        tc_queue_entries = [(0, 'UCAST', 0), (0, 'MULTICAST', 0),
                            (1, 'UCAST', 1), (2, 'MULTICAST', 1),
                            (2, 'UCAST', 2), (3, 'MULTICAST', 2)]
        tc_queue_id = create_map_example('tc-to-queue-map', tc_queue_entries)
        if tc_queue_id is None:
            print 'Failed to create tc-to-queue map'
            return False
        get_map_example('tc-to-queue-map', tc_queue_id)
    dot1p_tc_entries = [(2, 0, 'RED'), (3, 1, 'GREEN'), (4, 5, 'RED')]
    dot1p_tc_id = create_map_example('dot1p-to-tc-color-map', dot1p_tc_entries)
    if dot1p_tc_id is None:
        print 'Failed to create dot1p-to-tc-color map'
        return False
    get_map_example('dot1p-to-tc-color-map', dot1p_tc_id)
    dscp_tc_entries = [(0, 0, 'GREEN'), (1, 1, 'YELLOW')]
    dscp_tc_id = create_map_example('dscp-to-tc-color-map', dscp_tc_entries)
    if dscp_tc_id is None:
        print 'Failed to create dscp-to-tc-color map'
        return False
    get_map_example('dscp-to-tc-color-map', dscp_tc_id)

    print 'Start to create storm control policer profiles'
    flood_storm_control_id = create_storm_control_policer_example(200)
    if flood_storm_control_id is None:
        print 'Falied to create flood storm control policer'
        return False

    bcast_storm_control_id = create_storm_control_policer_example(300)
    if bcast_storm_control_id is None:
        print 'Failed to create unicast storm control policer'
        return False

    mcast_storm_control_id = create_storm_control_policer_example(500)
    if mcast_storm_control_id is None:
        print 'Failed to create unicast storm control policer'
        return False

    print 'Setup port ingress profile with maps and policers'
    attr_entries = [('dot1p-to-tc-color-map', dot1p_tc_id),
                    ('dscp-to-tc-color-map', dscp_tc_id),
                    ('flood_storm_control', flood_storm_control_id),
                    ('broadcast_storm_control', bcast_storm_control_id),
                    ('multicast_storm_control', mcast_storm_control_id)]
    if tc_queue_id is not None:
        attr_entries.append(('tc-to-queue-map', tc_queue_id))
    port_ing_obj = nas_qos.IngPortCPSObj(ifname=port_name,
                                         list_of_attr_value_pairs=attr_entries)
    upd = ('set', port_ing_obj.data())
    if cps_utils.CPSTransaction([upd]).commit() == False:
        print 'Failed to set map and policer for port %s' % port_name
        return False

    return_data_list = []
    ret = cps.get([port_ing_obj.data()], return_data_list)
    if ret:
        print '#### Port Ingress Porfile ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.IngPortCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'
        return False

    print 'Reset map and policer in port ingress profile'
    attr_entries = [('dot1p-to-tc-color-map', 0),
                    ('dscp-to-tc-color-map', 0),
                    ('flood_storm_control', 0),
                    ('broadcast_storm_control', 0),
                    ('multicast_storm_control', 0)]
    if tc_queue_id is not None:
        attr_entries.append(('tc-to-queue-map', 0))
    port_ing_obj = nas_qos.IngPortCPSObj(ifname=port_name,
                                         list_of_attr_value_pairs=attr_entries)
    upd = ('set', port_ing_obj.data())
    if cps_utils.CPSTransaction([upd]).commit() == False:
        print 'Failed to set map and policer for port %s' % port_name
        return False

    return_data_list = []
    ret = cps.get([port_ing_obj.data()], return_data_list)
    if ret:
        print '#### Port Ingress Porfile ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.IngPortCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'
        return False

    print 'Start to delete port-ingress map profiles'
    if tc_queue_id is not None:
        ret = delete_map_example('tc-to-queue-map', tc_queue_id)
        if ret == False:
            print 'Failed to delete tc-to-queue map'
            return False
    ret = delete_map_example('dot1p-to-tc-color-map', dot1p_tc_id)
    if ret == False:
        print 'Failed to delete dot1p-to-tc-color map'
        return False
    ret = delete_map_example('dscp-to-tc-color-map', dscp_tc_id)
    if ret == False:
        print 'Failed to delete dscp-to-tc-color map'
        return False

    print 'Start to delete storm-control policer profiles'
    delete_policer_example(flood_storm_control_id)
    delete_policer_example(bcast_storm_control_id)
    delete_policer_example(mcast_storm_control_id)

    return True


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


def scheduler_profile_delete_example(sched_id):
    sched_obj = nas_qos.SchedulerCPSObj(sched_id=sched_id)
    upd = ('delete', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        print 'Scheduler profile delete failed'
        return None

    print 'Return = ', ret_cps_data
    print 'Successfully deleted Scheduler id = ', sched_id

    return ret_cps_data


def test_qos_port_egress(port_name):
    print 'Test qos port-egress for port %s' % port_name
    """
    print 'Start to create port-egress map profiles'
    tc_color_dot1p_entries = [(0, 'RED', 0), (0, 'GREEN', 1), (5, 'RED', 2)]
    tc_color_dot1p_id = create_map_example('tc-color-to-dot1p-map',
                                            tc_color_dot1p_entries)
    if tc_color_dot1p_id == None:
        print 'Failed to cread tc-color-to-dot1p map'
        return False
    get_map_example('tc-color-to-dot1p-map', tc_color_dot1p_id)
    tc_color_dscp_entries = [(0, 'GREEN', 0), (1, 'YELLOW', 1), (2, 'RED', 2)]
    tc_color_dscp_id = create_map_example('tc-color-to-dscp-map',
                                           tc_color_dscp_entries)
    if tc_color_dscp_id == None:
        print 'Failed to cread tc-color-to-dscp map'
        return False
    get_map_example('tc-color-to-dscp-map', tc_color_dscp_id)
    """

    print 'Start to create scheduler profile'
    sched_id = scheduler_profile_create_example(
        'WRR', 50, 10000, 200, 50000, 200)
    if sched_id is None:
        print 'Failed to create scheduler profile'
        return False
    scheduler_profile_get_example(sched_id)

    print 'Setup port egress porfile with maps and scheduler'
    attr_entries = [  # ('tc-color-to-dot1p-map', tc_color_dot1p_id),
        #('tc-color-to-dscp-map', tc_color_dscp_id),
        ('scheduler-profile-id', sched_id)]
    port_eg_obj = nas_qos.EgPortCPSObj(ifname=port_name,
                                       list_of_attr_value_pairs=attr_entries)
    upd = ('set', port_eg_obj.data())
    if cps_utils.CPSTransaction([upd]).commit() == False:
        print 'Failed to set maps and scheduler for port %s' % port_name
        return False
    return_data_list = []
    ret = cps.get([port_eg_obj.data()], return_data_list)
    if ret:
        print '#### Port Egress Profile ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.EgPortCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'
        return False

    print 'Reset map and scheduler in port egress profile'
    attr_entries = [  # ('tc-color-to-dot1p-map', 0),
        #('tc-color-to-dscp-map', 0),
        ('scheduler-profile-id', 0)]
    port_eg_obj = nas_qos.EgPortCPSObj(ifname=port_name,
                                       list_of_attr_value_pairs=attr_entries)
    upd = ('set', port_eg_obj.data())
    if cps_utils.CPSTransaction([upd]).commit() == False:
        print 'Failed to set maps and scheduler for port %s' % port_name
        return False
    return_data_list = []
    ret = cps.get([port_eg_obj.data()], return_data_list)
    if ret:
        print '#### Port Egress Profile ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.EgPortCPSObj(cps_data=cps_ret_data)
            m.print_obj()
    else:
        print 'Error in get'
        return False

    """
    print 'Start to delete port-egress map profiles'
    ret = delete_map_example('tc-color-to-dot1p-map', tc_color_dot1p_id)
    if ret == False:
        print 'Failed to delete tc-color-to-dot1p map'
        return False
    ret = delete_map_example('tc-color-to-dscp-map', tc_color_dscp_id)
    if ret == False:
        print 'Failed to delete tc-color-to-dscp map'
        return False
    """

    print 'Start to delete scheduler profile'
    ret = scheduler_profile_delete_example(sched_id)
    if ret is None:
        print 'Failed to delete scheduler profile'
        return False

    return True

if __name__ == '__main__':
    if len(sys.argv) >= 2:
        port_name = sys.argv[1]
    else:
        port_name = get_first_phy_port()
    if test_qos_port_ingress(port_name) == False:
        exit()
    if test_qos_port_egress(port_name) == False:
        exit()
