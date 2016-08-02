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


def create_ing_map(in_map_name, entries):
    map_obj = nas_qos.MapCPSObjs(in_map_name, entry_list=entries)
    if map_obj.commit() == False:
        print "Failed to create " + in_map_name
        return None

    print "Successfully created " + in_map_name + " " + str(map_obj.get_map_id())
    return map_obj.get_map_id()


def set_port_ing_map(port_name, map_id, in_map_name):
    inp = nas_qos.IngPortCPSObj(ifname=port_name)
    inp.set_attr(in_map_name, map_id)
    upd = ('set', inp.data())
    if cps_utils.CPSTransaction([upd]).commit() == False:
        print "Failed to set " + in_map_name + " for port " + port_name
    else:
        print "Successfully set " + in_map_name + " for port " + port_name


def create_egr_map(eg_map_name, entries):
    map_obj = nas_qos.MapCPSObjs(eg_map_name, entry_list=entries)
    if map_obj.commit() == False:
        print "Failed to create " + eg_map_name
        return None

    print "Successfully created " + eg_map_name + " " + str(map_obj.get_map_id())
    return map_obj.get_map_id()


def set_port_egr_map(port_name, map_id, eg_map_name):
    # Set the TC to Q map for port e00-1
    op = nas_qos.EgPortCPSObj(ifname=port_name)
    op.set_attr(eg_map_name, map_id)
    upd = ('set', op.data())
    if cps_utils.CPSTransaction([upd]).commit() == False:
        print "Failed to set " + eg_map_name + " for port " + port_name
    else:
        print "Successfully set " + eg_map_name + " for port " + port_name


def modify_ing_map(in_map_name, map_id, mod_list, new_list):
    # Modify existing map table entry
    m = nas_qos.MapCPSObjs(in_map_name, map_id=map_id)
    mod_list = [(1, 2, 'GREEN'), (2, 6, 'RED')]
    new_list = [(5, 3, 'YELLOW'), (6, 4, 'RED')]
    m.mod_entries(mod_list)
    m.new_entries(new_list)
    if m.commit() == False:
        print "Failed to modify " + in_map_name
    else:
        print "Successfully modified " + in_map_name


def get_map(map_name, map_id):
    # CPS Data get existing map table
    m = nas_qos.MapCPSObjs(map_name, map_id=map_id)
    ret_data_list = []
    cps.get([m.data_map()], ret_data_list)
    print "## CPS data for " + map_name
    print ret_data_list


if __name__ == '__main__':

    # Each tuple in the list has the following form (dot1p, tc, color)
    d2tc_entries = [(0, 0, 'RED'),
                    (1, 1, 'GREEN'),
                    (2, 5, 'RED')]
    d2tc_id = create_ing_map('dot1p-to-tc-color-map', d2tc_entries)
    set_port_ing_map('e101-003-0', d2tc_id, 'dot1p-to-tc-color-map')

    # Each tuple in the list has the following form (tc, queue-type,
    # queue-number)
    tc2q_entries = [(0, 'UCAST', 0), (0, 'MULTICAST', 0),
                    (2, 'UCAST', 1), (2, 'MULTICAST', 1),
                    (3, 'UCAST', 2), (3, 'MULTICAST', 2)]
    tc2q_id = create_egr_map('tc-to-queue-map', tc2q_entries)
    set_port_egr_map('e101-004-0', tc2q_id, 'tc-to-queue-map')

    mod_list = [(1, 2, 'GREEN'), (2, 6, 'RED')]
    new_list = [(5, 3, 'YELLOW'), (6, 4, 'RED')]
    modify_ing_map('dot1p-to-tc-color-map', d2tc_id, mod_list, new_list)

    ''' not supported yet
    # Each tuple in the list has the following form (tc, color, dot1p)
    tc2d_entries = [(0, 'RED', 0),
                    (0, 'GREEN', 1),
                    (5, 'RED', 2)]
    tc2d_id = create_egr_map('tc-color-to-dot1p-map', tc2d_entries)
    set_port_egr_map('e101-001-0', tc2d_id, 'tc-color-to-dot1p-map')
    '''

    get_map('dot1p-to-tc-color-map', d2tc_id)
    get_map('tc-to-queue-map', tc2q_id)
    '''
    get_map('tc-color-to-dot1p-map', d2tc_id)
    '''

    tc2pg_entries = [(0, 0), (1, 0), (2, 1), (3, 1)]
    tc2pg_id = create_ing_map('tc-to-priority-group-map', tc2pg_entries)
    set_port_ing_map('e101-005-0', tc2pg_id, 'tc-to-priority-group-map')

    get_map('tc-to-priority-group-map', tc2pg_id)

    pfc2q_entries = [(0, 'UCAST', 0), (1, 'UCAST', 1), (2, 'UCAST', 2), (3, 'UCAST', 3),
                     (0, 'MULTICAST', 0), (1, 'MULTICAST', 1), (2, 'MULTICAST', 2), (3, 'MULTICAST', 3),
                     (4, 'UCAST', 4), (5, 'UCAST', 5), (6, 'UCAST', 6), (7, 'UCAST', 7) ]
    pfc2q_id = create_egr_map('pfc-priority-to-queue-map', pfc2q_entries)
    set_port_egr_map('e101-005-0', pfc2q_id, 'pfc-priority-to-queue-map')

    get_map('pfc-priority-to-queue-map', pfc2q_id)


