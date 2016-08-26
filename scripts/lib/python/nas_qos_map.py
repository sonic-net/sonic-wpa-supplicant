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
import nas_qos_utils as utl


class MapCPSObjs:

    """
    Utility class to build a set of CPS Objects for the Map Table
    and each Map Entry based on the QoS Map models with attributes populated
    from the values passed as inputs.

    The resulting set of CPS Objects can be applied by calling the
    commit () method.
    """

    type_map = {
        'switch-id': ('leaf', 'uint32_t'),
        'id': ('leaf', 'uint64_t'),
        'dot1p': ('leaf', 'uint8_t'),
        'tc': ('leaf', 'uint8_t'),
        'dscp': ('leaf', 'uint8_t'),
        'color': ('leaf', 'enum', 'base-qos:packet-color:'),
        'queue-number': ('leaf', 'uint32_t'),
        'type': ('leaf', 'enum', 'base-qos:queue-type:'),
        'priority-group': ('leaf', 'int8_t'),
        'pfc-priority': ('leaf', 'uint8_t'),
    }

    @classmethod
    def get_type_map(cls):
        return cls.type_map

    map_attr_names = {
        'dot1p-to-tc-map': (1, 'dot1p', 'tc'),
        'dot1p-to-tc-color-map': (1, 'dot1p', 'tc', 'color'),
        'dscp-to-tc-map': (1, 'dscp', 'tc'),
        'dscp-to-tc-color-map': (1, 'dscp', 'tc', 'color'),
        'tc-to-queue-map': (2, 'tc', 'type', 'queue-number'),
        'tc-color-to-dot1p-map': (2, 'tc', 'color', 'dot1p'),
        'tc-color-to-dscp-map': (2, 'tc', 'color', 'dscp'),
        'tc-to-priority-group-map': (1, 'tc', 'priority-group'),
        'priority-group-to-pfc-priority-map': (2, 'priority-group', 'pfc-priority'),
        'pfc-priority-to-queue-map': (2, 'pfc-priority', 'type', 'queue-number'),
    }

    @classmethod
    def get_attr_names(cls, map_type):
        return cls.map_attr_names[map_type][1:]

    def get_entry_key_num(self, map_type):
        return self.map_attr_names[map_type][0]

    # Newer map tables do not have SWITCH_ID field
    old_maps = ['dot1p-to-tc-map', 'dot1p-to-tc-color-map',
		'dscp-to-tc-map', 'dscp-to-tc-color-map',
		'tc-to-queue-map', 'tc-color-to-dot1p-map',
		'tc-color-to-dscp-map']	
    @classmethod
    def get_old_maps(cls):
        return cls.old_maps

    def __init__(self, yang_map_name, entry_list=[], map_id=None,
                 switch_id=0, cps_data=None):
        self.yang_name = yang_map_name
        self.yang_entry_name = yang_map_name + '/entry'

        if cps_data is not None:
            self.cps_data = cps_data
            return

        self.cps_data = None
        self.switch_id = switch_id

        self.map_id = map_id
        if map_id is None:
            self.map_id = self.__create_map_table()

        self.new_entry_list = []
        self.mod_entry_list = []
        self.del_entry_list = []
        for pair in entry_list:
            entry_obj_wr = self.__form_map_entry_obj(pair)
            self.new_entry_list.append(entry_obj_wr)

    def __create_map_table(self):
        map_wr = utl.CPSObjWrp(self.yang_name, self.get_type_map())
        if self.yang_name in self.get_old_maps():
            map_wr.add_leaf_attr('switch-id', self.switch_id)

        upd = ('create', map_wr.get())
        ret_data_list = cps_utils.CPSTransaction([upd]).commit()

        if ret_data_list == False:
            print "Map Table Create failed - " + self.yang_name
            return None

        self.map_result = ret_data_list[0]
        map_id = utl.extract_cps_attr(self, ret_data_list[0], 'id')
        return map_id

    def __form_map_entry_obj(self, pair):
        val_num = 0
        entry_wr = utl.CPSObjWrp(self.yang_entry_name, self.get_type_map())
        if self.yang_name in self.get_old_maps():
            entry_wr.add_leaf_attr(
            	'switch-id',
            	self.switch_id,
            	utl.obj_path(self.yang_name))
        if self.get_map_id() is None:
            raise RuntimeError(
                'Map Entry needs Map Table to be created first')
        entry_wr.add_leaf_attr(
            'id',
            self.get_map_id(),
            utl.obj_path(self.yang_name))
        if isinstance(pair, tuple):
            names = self.get_attr_names(self.yang_name)
            for val in pair:
                entry_wr.add_leaf_attr(names[val_num], val)
                val_num += 1
        elif isinstance(pair, dict):
            for name, val in pair.items():
                entry_wr.add_leaf_attr(name, val)
        return entry_wr

    def new_entries(self, entry_list):
        for pair in entry_list:
            entry_obj_wr = self.__form_map_entry_obj(pair)
            self.new_entry_list.append(entry_obj_wr)

    def mod_entries(self, entry_list):
        for pair in entry_list:
            entry_obj_wr = self.__form_map_entry_obj(pair)
            self.mod_entry_list.append(entry_obj_wr)

    def del_entries(self, entry_list):
        for pair in entry_list:
            entry_obj_wr = self.__form_map_entry_obj(pair)
            self.del_entry_list.append(entry_obj_wr)

    def data_map(self):
        entry_wr = utl.CPSObjWrp(self.yang_name, self.get_type_map())
        if self.yang_name in self.get_old_maps():
            entry_wr.add_leaf_attr('switch-id', self.switch_id)
        entry_wr.add_leaf_attr('id', self.map_id)
        return entry_wr.get()

    def get_map_id(self):
        return self.map_id

    def commit(self):
        c = cps_utils.CPSTransaction()
        for entry_wr in self.new_entry_list:
            c.create(entry_wr.get())
        for entry_wr in self.mod_entry_list:
            c.set(entry_wr.get())
        for entry_wr in self.del_entry_list:
            c.delete(entry_wr.get())
        ret_data_list = c.commit()
        if ret_data_list == False:
            return False
        return ret_data_list

    def delete(self):
        ret_data_list = []
        cps.get([self.data_map()], ret_data_list)
        c = cps_utils.CPSTransaction()

        map_id = utl.extract_cps_attr(self, ret_data_list[0], 'id')
        entries = utl.extract_cps_attr(self, ret_data_list[0], 'entry')
        print "Trying to delete Map with entries ", entries

        key_num = self.get_entry_key_num(self.yang_name)
        for k, entry in entries.iteritems():
            entry_attr_names = self.get_attr_names(self.yang_name)
            val_list = []
            for idx in xrange(0, key_num):
                val = utl.extract_cps_attr(
                    self,
                    entry,
                    entry_attr_names[idx],
                    self.yang_entry_name)
                val_list.append(val)
            entry_wr = self.__form_map_entry_obj(tuple(val_list))
            c.delete(entry_wr.get())

        c.delete(self.data_map())
        return c.commit()

    def print_obj(self):
        """
        Print the contents of CPS Object in a user friendly format
        """
        if self.cps_data is None:
            return
        attr_name = 'id'
        val = utl.extract_cps_attr(self, self.cps_data, attr_name)
        print '%-30s %d' % (attr_name, val)
        attr_name = 'entry'
        entries = utl.extract_cps_attr(self, self.cps_data, attr_name)
        for k, entry in entries.iteritems():
            entry_attr_names = self.get_attr_names(self.yang_name)
            entry_attr_str = ''
            for name in entry_attr_names:
                val = utl.extract_cps_attr(
                    self,
                    entry,
                    name,
                    self.yang_entry_name)
                entry_attr_str += '%s: %s ' % (name, val)
            print '%-30s %s' % (attr_name, entry_attr_str)
