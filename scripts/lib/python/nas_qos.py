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

from nas_qos_meter import *
from nas_qos_map import *
from nas_qos_port import *
from nas_qos_queue import *
from nas_qos_queue_stat import *
from nas_qos_wred import *
from nas_qos_scheduler import *
from nas_qos_sched_group import *
from nas_qos_buffer_pool import *
from nas_qos_buffer_profile import *
from nas_qos_priority_group import *
from nas_qos_priority_group_stat import *
from nas_qos_buffer_pool_stat import *

import nas_os_utils as os_utl
import cps_utils
import cps


def get_port_queue_id(ifname, queue_number, queue_type):
    """
    Get Queue id
    Sample: get_port_queue_id('e101-032-0', 1, 'UCAST')
    """
    return_data_list = []
    port_id = os_utl.if_nametoindex(ifname)

    queue_obj = QueueCPSObj(
        queue_type=queue_type,
        queue_number=queue_number,
        port_id=port_id)
    ret = cps.get([queue_obj.data()], return_data_list)

    if ret:
        for cps_ret_data in return_data_list:
            m = QueueCPSObj(
                port_id=port_id,
                queue_type=queue_type,
                queue_number=queue_number,
                cps_data=cps_ret_data)
            if (m.extract_attr('type') == queue_type and
                    m.extract_attr('queue-number') == queue_number):
                return m.extract_id()
    else:
        print 'Error in get'

    return None


def get_queue_parent_sched_group_id(ifname, queue_number, queue_type):
    """
    Get the parent scheduler group id based on the {port + q_number + type}
    @return scheduler_group_id or None
    Sample: get_queue_parent_sched_group_id ('e101-032-0', 1, 'UCAST')
    """
    # find the Qid first
    qid = get_port_queue_id(ifname, queue_number, queue_type)

    return get_parent_sched_group_id(ifname, qid)


def get_parent_sched_group_id(ifname, child_id):
    """
    Get the parent scheduler group id
    @ifname interface nae
    @child_id queue_id or scheduler_group_id
    @return parent scheduler_group_id or None
    Sample: get_parent_sched_group_id('e101-032-0', 281474976711181)

    """
    # find the parent with a matching child qid
    sg_info = {}
    return_data_list = []
    sg_obj = SchedGroupCPSObj(sg_id=None, port_name=ifname,
                              level=None)
    ret = cps.get([sg_obj.data()], return_data_list)
    if ret:
        for cps_ret_data in return_data_list:
            m = SchedGroupCPSObj(cps_data=cps_ret_data)
            sg_id = m.extract_attr('id')
            child_list = m.extract_attr('child-list')
            if (child_list and
                    child_id in child_list):
                return sg_id
    else:
        print 'Error in get'

    return None


def create_storm_control_policer(peak_rate):
    meter_obj = MeterCPSObj(meter_type='PACKET', pir=peak_rate)
    meter_obj.set_attr('mode', 'STORM_CONTROL')
    upd = ('create', meter_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        raise RuntimeError('Policer creation failed')

    meter_obj = MeterCPSObj(cps_data=ret_cps_data[0])
    meter_id = meter_obj.extract_id()

    print 'Successfully created Policer Id = %d' % meter_id
    return meter_id


def delete_policer(meter_id):
    meter_obj = MeterCPSObj(meter_id=meter_id)
    upd = ('delete', meter_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        raise RuntimeError('Policer delete failed')

    print 'Successfully deleted Policer Id = %d' % meter_id

def scheduler_profile_create(
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
    sched_obj = SchedulerCPSObj(map_of_attr=attr_list)
    upd = ('create', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        raise RuntimeError('Scheduler profile creation failed')

    sched_obj = SchedulerCPSObj(cps_data=ret_cps_data[0])
    sched_id = sched_obj.extract_id()
    print 'Successfully installed Scheduler profile id = ', sched_id

    return sched_id


def scheduler_profile_delete(sched_id):
    sched_obj = SchedulerCPSObj(sched_id=sched_id)
    upd = ('delete', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        raise RuntimeError('Scheduler profile delete failed')

    print 'Successfully deleted Scheduler id = ', sched_id


def set_port_ingress_bcast_storm_control(port_name, policer_id):
    attr_entries = [('broadcast_storm_control', policer_id)]
    port_ing_obj = IngPortCPSObj(ifname=port_name,
                                         list_of_attr_value_pairs=attr_entries)
    upd = ('set', port_ing_obj.data())
    if cps_utils.CPSTransaction([upd]).commit() == False:
        raise RuntimeError('Failed to set policer for port %s' % port_name)

def set_port_egress_scheduler_id(port_name, sched_id):
    attr_entries = [('scheduler-profile-id', sched_id)]
    port_eg_obj = EgPortCPSObj(ifname=port_name,
                                       list_of_attr_value_pairs=attr_entries)
    upd = ('set', port_eg_obj.data())
    if cps_utils.CPSTransaction([upd]).commit() == False:
        raise RuntimeError('Failed to set scheduler for port %s' % port_name)


