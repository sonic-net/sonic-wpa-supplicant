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

import os
import bisect
import nas_qos
import xml.etree.ElementTree as ET
import cps
import cps_utils
import time
import copy
import nas_os_if_utils
import cps_object
import sys
import syslog

dbg_on = False
target_cfg_path = '/etc/sonic'
err_detected = False


def dbg_print(*args):
    if dbg_on:
        print (str(args))


def get_cfg():
    return target_cfg_path + '/base_qos_init.xml'

map_types = [
    'dot1p-to-tc-color-map', 'dscp-to-tc-color-map', 'tc-color-to-dot1p-map',
    'tc-color-to-dscp-map', 'tc-to-queue-map']

unsupported_map_types = ['tc-color-to-dot1p-map', 'tc-color-to-dscp-map']


def init_switch_globals(xnode_glb, lookup_map, lookup_sched_prof):
    global err_detected
    for xnode_obj in xnode_glb:
        # Maps
        if xnode_obj.tag in map_types:
            if xnode_obj.tag in unsupported_map_types:
                continue
            syslog.syslog("Creating {0}".format(xnode_obj.tag))
            try:
                map_id = create_map(xnode_obj)
                lookup_map[xnode_obj.attrib['tag']] = map_id
            except RuntimeError as r:
                syslog.syslog( 'Failed: ' + xnode_obj.attrib['tag'] + ' ' + str(r) + ';'
                               'Continuing with rest of the initialiation')
                err_detected = True
        # Scheduler Profiles
        elif xnode_obj.tag == 'scheduler-profile':
            prof_id = create_scheduler_profile(xnode_obj)
            lookup_sched_prof[xnode_obj.attrib['tag']] = prof_id


def init_fp_ports(xnode_fp, lookup_map, lookup_sched_prof):
    init_all_ports_of_type('ianaift:ethernetCsmacd', xnode_fp, lookup_map, lookup_sched_prof)


def init_cpu_ports(xnode_cpu, lookup_map, lookup_sched_prof):
    init_all_ports_of_type('base-if:cpu', xnode_cpu, lookup_map, lookup_sched_prof)


def init_all_ports_of_type(
        iftype, xnode_port, lookup_map, lookup_sched_prof):
    # keep track of configured port in case more interfaces are created
    # while we are initializing default NAS-QOS
    done_ifs = []
    check_new_intf = True
    while (check_new_intf):
        check_new_intf = False
        if iftype == 'base-if:cpu':
            ifs = nas_os_if_utils.nas_os_cpu_if()
        else:
            ifs = nas_os_if_utils.nas_os_if_list()
        for intf in ifs:
            if intf in done_ifs:
                continue

            # add to configured port list
            done_ifs.append(intf)
            check_new_intf = True

            obj = cps_object.CPSObject(obj=intf)
            try:
                _iftype = obj.get_attr_data('if/interfaces/interface/type')
            except ValueError:
                continue
            if _iftype != iftype:
                continue
            ifname = obj.get_attr_data('if/interfaces/interface/name')
            ifidx = obj.get_attr_data('dell-base-if-cmn/if/interfaces/interface/if-index')

            init_port(ifidx, ifname, xnode_port, lookup_map, lookup_sched_prof)


def init_port(ifidx, ifname, xnode_port, lookup_map, lookup_sched_prof):
    global err_detected
    syslog.syslog("==== Initializing port {0} =====".format(ifname))
    for xnode_obj in xnode_port:
        # Maps
        if xnode_obj.tag == 'ingress' or \
           xnode_obj.tag == 'egress':
            for xnode_attr in xnode_obj:
                if xnode_attr.tag in map_types:
                    if xnode_attr.tag in unsupported_map_types:
                        continue
                    syslog.syslog(" > Applying Map {0}".format(xnode_attr.text))
                    try:
                        map_id = lookup_map[xnode_attr.text]
                        bind_obj_to_port(
                            xnode_attr.tag,
                            map_id,
                            xnode_obj.tag,
                            ifname)
                    except RuntimeError as r:
                        syslog.syslog('    Failed: ' + xnode_attr.text + ' ' + str(r) + ';'
                                      '    Continuing with rest of the initialization')
                        err_detected = True
                    except KeyError as k:
                        syslog.syslog('    Failed - Could not find Map obj named: ' + str(k) + ';'
                                      '    Continuing with rest of the initialization')
                        err_detected = True
        # Scheduler Hierarchy
        elif xnode_obj.tag == 'scheduler-hierarchy-tree':
            syslog.syslog(" > Creating scheduler tree")
            init_sched_tree_on_port(
                xnode_obj,
                lookup_sched_prof,
                ifidx,
                ifname)


def bind_obj_to_port(attr_name, obj_id, direction, ifname):
    dbg_print('{0} Binding on port {1}: {2} = {3}'.format(
        direction, ifname, attr_name, str(obj_id)))
    port = None
    if direction == 'ingress':
        port = nas_qos.IngPortCPSObj(ifname=ifname,
                                     list_of_attr_value_pairs=[(attr_name, obj_id)])
    else:
        port = nas_qos.EgPortCPSObj(ifname=ifname,
                                    list_of_attr_value_pairs=[(attr_name, obj_id)])
    upd = ('set', port.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        raise RuntimeError(
            "Failed to bind " +
            attr_name +
            " to port " +
            ifname)


def init_sched_tree_on_port(
        xnode_sched_tree, lookup_sched_prof, ifidx, ifname):

    # 1st pass - Read all scheduler groups and scheduler profile settings
    # for CPU or front-panel port
    sg_info = read_sched_grp(ifidx, ifname)

    # read scheduler profile from .xml
    sg_lookup = {}
    for xnode_level in xnode_sched_tree:
        cur_level = int(xnode_level.attrib['level'])
        if (xnode_level.attrib['leaf'] != "true"):
            # SG nodes
            for xnode_sched_grp in xnode_level:
                sg_number = int(xnode_sched_grp.attrib['number'])
                profile = xnode_sched_grp.attrib['scheduler-profile']
                sg_id = get_sched_grp(cur_level, sg_number, sg_info)
                if (sg_id is None):
                    continue
                if (dbg_on):
                    print "bind sg_id:{0} of level {1}:{2} to profile {3}".format(
                       sg_id, cur_level, sg_number, profile)
                bind_sched_profile(
                    sg_id,
                    profile,
                    lookup_sched_prof,
                    str(cur_level) + str(sg_number))
        else:
            # Queues
            for xnode_queue in xnode_level:
                queue_number = int(xnode_queue.attrib['number'])
                profile = xnode_queue.attrib['scheduler-profile']
                '''
                queue_id = get_port_queue_id (ifidx, xnode_queue.attrib ['type'],
                                      queue_number, ifname)
                '''
                bind_q_profile(
                    (ifidx, xnode_queue.attrib['type'], queue_number),
                    profile, lookup_sched_prof)

    if (dbg_on):
        print "======After configuration, New setting:"
        read_sched_grp(ifidx, ifname)


def get_sched_grp(level, sg_number, sg_info):
    return sg_info[level][sg_number]


def read_sched_grp(ifidx, ifname):
    sg_info = {}
    return_data_list = []
    sg_obj = nas_qos.SchedGroupCPSObj(sg_id=None, port_name=ifname,
                                      level=None)
    ret = cps.get([sg_obj.data()], return_data_list)
    if ret:
        #print '#### Scheduler Group Show ####'
        for cps_ret_data in return_data_list:
            m = nas_qos.SchedGroupCPSObj(cps_data=cps_ret_data)
            if (dbg_on):
                m.print_obj()
            level = m.extract_attr('level')
            sg_id = m.extract_attr('id')
            if (level in sg_info):
                bisect.insort(sg_info[level], sg_id)
            else:
                sg_info[level] = [sg_id]

    else:
        syslog.syslog('Error in get port {0} attributes'.format(ifname))

    return sg_info


def create_map(xnode_map_tbl):
    entries = []
    for xnode_map_entry in xnode_map_tbl:
        e = get_map_entry(xnode_map_tbl, xnode_map_entry)
        entries.extend(e)
    dbg_print('Cfg entries= {0}'.format(entries))
    map_obj = nas_qos.MapCPSObjs(xnode_map_tbl.tag, entry_list=entries)
    if map_obj.commit() == False:
        raise RuntimeError(
            "Failed to create " +
            xnode_map_tbl.tag +
            xnode_map_tbl.attrib[
                'tag'])

    map_id = map_obj.get_map_id()
    dbg_print(' Map {0} = {1}'.format(xnode_map_tbl.attrib['tag'], map_id))
    return map_id


def create_scheduler_profile(xnode_profile):
    attrs = copy.deepcopy(xnode_profile.attrib)
    del attrs['tag']
    sched_obj = nas_qos.SchedulerCPSObj(map_of_attr=attrs)
    upd = ('create', sched_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        raise RuntimeError(
            'Scheduler profile {0} creation failed'.format(xnode_profile.attrib['tag']))

    sched_obj = nas_qos.SchedulerCPSObj(cps_data=ret_cps_data[0])
    sched_id = sched_obj.extract_id()
    dbg_print(' Scheduler profile {0} = {1}'.format(
        xnode_profile.attrib['tag'], sched_id))

    return sched_id


def create_sched_grp(port_name, level, sched_grp_name):
    sg_obj = nas_qos.SchedGroupCPSObj(port_name, level)
    upd = ('create', sg_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()

    if ret_cps_data == False:
        raise RuntimeError(
            'Port {0} Scheduler Group {1} creation failed',
            port_name,
            sched_grp_name)

    sg_obj = nas_qos.SchedGroupCPSObj(cps_data=ret_cps_data[0])
    sg_id = sg_obj.extract_id()
    return sg_id


def add_child_to_sched_grp(sg_id, child_list, sched_grp_name):
    sg_obj = nas_qos.SchedGroupCPSObj(sg_id=sg_id,
                                      chld_id_list=child_list)
    upd = ('set', sg_obj.data())
    ret_cps_data = cps_utils.CPSTransaction([upd]).commit()
    if ret_cps_data == False:
        raise RuntimeError(
            "FAILED to add child nodes to {0}".format(sched_grp_name))
    dbg_print(
        "         > Successfully added child nodes to {0}".format(sched_grp_name))


def bind_q_profile(key_tuple, profile_name, sched_lookup):
    q_obj = nas_qos.QueueCPSObj(
        port_id=key_tuple[0], queue_type=key_tuple[1],
        queue_number=key_tuple[2])
    q_obj.set_attr('scheduler-profile-id', sched_lookup[profile_name])
    upd = ('set', q_obj.data())
    r = cps_utils.CPSTransaction([upd]).commit()
    if r == False:
        raise RuntimeError(
            'Unable to set scheduler profile {0} for {1} Queue {2}'.format(profile_name,
                                                                           key_tuple[
                                                                           1],
                                                                           key_tuple[2]))
    dbg_print(
        '         > Successfully set scheduler profile {0} for  {1} Queue {2}'.format(profile_name,
                                                                                      key_tuple[
                                                                                      1],
                                                                                      key_tuple[2]))


def bind_sched_profile(sg_id, prof_name, sched_lookup, sched_grp_name):
    sched_id = sched_lookup[prof_name]
    sg_obj = nas_qos.SchedGroupCPSObj(sg_id=sg_id, sched_id=sched_id)
    upd = ('set', sg_obj.data())
    r = cps_utils.CPSTransaction([upd]).commit()
    if r == False:
        raise RuntimeError(
            'Unable to set scheduler profile {0} for Scheduler Group {1}'.format(
                prof_name,
                sched_grp_name))
    dbg_print(
        '         > Successfully set scheduler profile {0} for Scheduler Group {1}'.format(
            prof_name,
            sched_grp_name))


def get_map_entry(xnode_map_tbl, xnode_map_entry):
    map_entry_dict = xnode_map_entry.attrib
    expand = False
    expand_attr = ""
    for attr, val in map_entry_dict.items():
        if val == 'all':
            expand = True
            expand_attr = attr

    if expand:
        out_entry_list = []
        for i in range(int(xnode_map_tbl.attrib['min']), (int(xnode_map_tbl.attrib['max'])) + 1):
            d = copy.deepcopy(map_entry_dict)
            d[expand_attr] = str(i)
            out_entry_list.append(d)
        return out_entry_list
    return [map_entry_dict]


def get_port_queue_id(ifidx, queue_type, q_num, ifname):
    queue_obj = nas_qos.QueueCPSObj(
        port_id=ifidx,
        queue_type=queue_type,
        queue_number=q_num)
    cps_data_list = []
    ret = cps.get([queue_obj.data()], cps_data_list)

    if ret == False:
        raise RuntimeError(
            'Failed to get {1} queue {2} id for port {0}({3})'.format(
                ifname,
                queue_type,
                q_num,
                ifidx))

    m = nas_qos.QueueCPSObj(None, None, None, cps_data=cps_data_list[0])
    queue_id = m.extract_id()
    dbg_print(
        "           > Intf {4}({0}) {1} Q{2} = QID 0x{3:X}".format(
            ifidx,
            queue_type,
            q_num,
            queue_id,
            ifname))
    return queue_id


def wait_for_register():
    ifidx = get_one_interface()
    time.sleep(1)

    p = nas_qos.IngPortCPSObj(ifindex=ifidx)
    r = []

    while cps.get([p.data()], r) == False:
        time.sleep(1)


def get_one_interface():
    ifs = nas_os_if_utils.nas_os_if_list()
    while not ifs:
        ifs = nas_os_if_utils.nas_os_if_list()
        time.sleep(1)

    obj = cps_object.CPSObject(obj=ifs[-1])
    return obj.get_attr_data('dell-base-if-cmn/if/interfaces/interface/if-index')

def init_interfaces(ifnames):
    if 'DN_QOS_CFG_PATH' in os.environ.keys():
        qos_cfg_path = os.environ['DN_QOS_CFG_PATH']
    else:
        qos_cfg_path = target_cfg_path

    xnode_root = ET.parse(get_cfg()).getroot()

    lookup_sched_prof = {}

    try:
        for xnode_obj in xnode_root:
            if xnode_obj.tag == 'SWITCH-GLOBALS':
                for xnode_glb_obj in xnode_obj:
                    if xnode_glb_obj.tag == 'scheduler-profile':
                        prof_id = create_scheduler_profile(xnode_glb_obj)
                        lookup_sched_prof[xnode_glb_obj.attrib['tag']] = prof_id
            elif xnode_obj.tag == 'FRONT-PANEL-PORTS':
                for xnode_port_obj in xnode_obj:
                    if xnode_port_obj.tag == 'scheduler-hierarchy-tree':
                        for ifname in ifnames:
                            ifidx = nas_os_if_utils.name_to_ifindex(ifname)
                            init_sched_tree_on_port(
                                xnode_port_obj,
                                lookup_sched_prof,
                                ifidx,
                                ifname)

    except RuntimeError as r:
        syslog.syslog("Runtime Error: " + str(r))
        sys.exit(1)

    if err_detected:
        sys.exit(1)


if __name__ == '__main__':

    if 'DN_QOS_CFG_PATH' in os.environ.keys():
        qos_cfg_path = os.environ['DN_QOS_CFG_PATH']
    else:
        qos_cfg_path = target_cfg_path

    wait_for_register()

    xnode_root = ET.parse(get_cfg()).getroot()

    lookup_map = {}
    lookup_sched_prof = {}

    try:
        for xnode_obj in xnode_root:
            if xnode_obj.tag == 'SWITCH-GLOBALS':
                init_switch_globals(xnode_obj, lookup_map, lookup_sched_prof)
            elif xnode_obj.tag == 'FRONT-PANEL-PORTS':
                init_fp_ports(xnode_obj, lookup_map, lookup_sched_prof)
            elif xnode_obj.tag == 'CPU-PORT':
                init_cpu_ports(xnode_obj, lookup_map, lookup_sched_prof)
            else:
                syslog.syslog("Unknown tag " + xnode_obj.tag)

    except RuntimeError as r:
        syslog.syslog("Runtime Error: " + str(r))
        sys.exit(1)

    if err_detected:
        sys.exit(1)
