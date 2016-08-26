
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*!
 * \file   nas_qos_switch_list.cpp
 * \brief  Managing a cache within QoS module for the list of switches in the system
 * \date   02-2015
 * \author
 */

#include <unordered_map>
#include "event_log.h"
#include "nas_qos_common.h"
#include "nas_qos_switch_list.h"
#include "nas_switch.h"
#include "nas_ndi_switch.h"

typedef std::unordered_map<uint_t, nas_qos_switch>  switch_list_t;
typedef switch_list_t::iterator switch_iter_t;

static switch_list_t nas_qos_switch_list_cache;


/**
 *  This function gets a switch instance from the cached switch list in QoS
 *  @Param switch_id
 *  @return QoS Switch Object pointer
 */
nas_qos_switch * nas_qos_get_switch(uint_t switch_id)
{
    try {
        /* get from our cache first */
        return &(nas_qos_switch_list_cache.at(switch_id));

    } catch (std::out_of_range) {

        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                     "Creating Switch id %d from inventory", switch_id);

        nas_qos_switch nas_qos_sw(switch_id);

        // Search the switch information from the config file
        const nas_switch_detail_t* sw =  nas_switch (switch_id);
        if (sw == NULL) {
            // No such switch id in our inventory
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Switch id %d does not exist", switch_id);
            return NULL;
        }

        for (size_t count = 0; count < sw->number_of_npus; count++)
            nas_qos_sw.add_npu (sw->npus[count]);

        /* init switch-wide queue info using one of the npu ids */
        if (sw->number_of_npus > 0) {
            if (ndi_switch_get_queue_numbers(sw->npus[0],
                    &nas_qos_sw.ucast_queues_per_port, &nas_qos_sw.mcast_queues_per_port,
                    &nas_qos_sw.total_queues_per_port, &nas_qos_sw.cpu_queues)
                    != STD_ERR_OK) {
                EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Failed to get global queue info for switch id %d",
                        switch_id);
                return NULL;
            }
        }

        if (nas_qos_add_switch(switch_id, nas_qos_sw) != STD_ERR_OK)
            return NULL;
        else
            return &(nas_qos_switch_list_cache.at(switch_id));
    }

    return NULL;
}

/**
 * This function add a switch instance to QoS switch list
 * @Param switch_id
 * @Param QoS switch instance
 * @return standard error code
 */
t_std_error nas_qos_add_switch (uint_t switch_id, nas_qos_switch& s)
{
    /* Do NOT allow overwrite of existing entry */
    switch_iter_t si = nas_qos_switch_list_cache.find(switch_id);
    nas_qos_switch *p = ((si != nas_qos_switch_list_cache.end())? &(si->second): NULL);

    if (p) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Switch id %d exists already", switch_id);
        return NAS_BASE_E_DUPLICATE;
    }

    // insert
    try {
        nas_qos_switch_list_cache.insert(std::make_pair(switch_id, std::move(s)));
    }
    catch (...) {
        EV_LOG_ERR(ev_log_t_QOS, 3, "NAS-QOS", "Failed to insert a new switch id %u", switch_id);
        return NAS_BASE_E_FAIL;
    }

    return STD_ERR_OK;
}

/**
 * This function removes a switch instance from QoS switch list
 * @Param switch_id
 * @Return standard error code
 */
t_std_error nas_qos_remove_switch (uint32_t switch_id)
{
    try {
        nas_qos_switch_list_cache.erase(switch_id);
    }
    catch (...) {
        EV_LOG_ERR(ev_log_t_QOS, 3, "NAS-QOS", "Failed to delete switch id %u", switch_id);
        return NAS_BASE_E_FAIL;
    }

    return STD_ERR_OK;
}

/**
 *  This function returns the switch pointer that contains the specified npu_id
 *  @Param npu id
 *  @Return Switch instance pointer if found
 */
nas_qos_switch * nas_qos_get_switch_by_npu(npu_id_t npu_id)
{
    // find the switch instance to which the NPU_id belongs
    nas_switch_id_t switch_id;
    if (nas_find_switch_id_by_npu(npu_id, &switch_id) != true) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "npu_id %u is not in any switch",
                     npu_id);
        return NULL;
    }

    return nas_qos_get_switch(switch_id);
}

/*
 * This function fills in the interface_ctrl_t structure given an ifindex
 * @Param ifindex
 * @Return True if the interface structure is properly filled; False otherwise
 */
bool nas_qos_get_port_intf(uint_t ifindex, interface_ctrl_t *intf_ctrl)
{
    /* get the npu id of the port */
    intf_ctrl->q_type = HAL_INTF_INFO_FROM_IF;
    intf_ctrl->if_index = ifindex;
    intf_ctrl->vrf_id = 0; //default vrf
    if (dn_hal_get_interface_info(intf_ctrl) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Cannot find NPU id for ifIndex: %d",
                        ifindex);
        return false;
    }

    return true;
}

