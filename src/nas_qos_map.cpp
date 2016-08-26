
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*!
 * \file   nas_qos_map.cpp
 * \brief  NAS QOS map Object
 */

#include "event_log.h"
#include "std_assert.h"
#include "nas_qos_common.h"
#include "nas_qos_map.h"
#include "dell-base-qos.h"
#include "nas_ndi_qos.h"
#include "nas_base_obj.h"
#include "nas_qos_switch.h"

nas_qos_map::nas_qos_map (nas_qos_switch* switch_p, nas_qos_map_type_t val)
           : base_obj_t (switch_p)
{
    type = val;
    map_id = 0;
}

const nas_qos_switch& nas_qos_map::get_switch()
{
    return static_cast<const nas_qos_switch&> (base_obj_t::get_switch());
}

void* nas_qos_map::alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m)
{
    // NAS Qos map does not allocate memory to save the incoming tentative attributes
    return this;
}

bool nas_qos_map::push_create_obj_to_npu (npu_id_t npu_id,
                                     void* ndi_obj)
{
    ndi_obj_id_t ndi_map_id;
    t_std_error rc;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Creating obj on NPU %d", npu_id);

    if ((rc = ndi_qos_create_map (npu_id,
                                  nas2ndi_qos_map_type(type),
                                  0, //map_list.size(),
                                  0, //&map_list[0],
                                  &ndi_map_id))
            != STD_ERR_OK)
    {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI QoS Map Create Failed"};
    }
    // Cache the new Map ID generated by NDI
    set_ndi_obj_id(npu_id, ndi_map_id);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "NPU %d creates Obj id %u",
                npu_id, ndi_map_id);

    return true;

}


bool nas_qos_map::push_delete_obj_to_npu (npu_id_t npu_id)
{
    t_std_error rc;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Deleting obj on NPU %d", npu_id);

    if ((rc = ndi_qos_delete_map(npu_id, ndi_obj_id(npu_id)))
        != STD_ERR_OK)
    {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI Map Delete Failed"};
    }

    return true;
}


t_std_error nas_qos_map::add_map_entry(nas_qos_map_entry_key_t key,
                                    nas_qos_map_entry & entry)
{
    map_entries * entries;

    switch (this->get_type()) {
    case NDI_QOS_MAP_DOT1P_TO_TC_COLOR:
    case NDI_QOS_MAP_DOT1P_TO_TC:
    case NDI_QOS_MAP_DOT1P_TO_COLOR:
    case NDI_QOS_MAP_DSCP_TO_TC_COLOR:
    case NDI_QOS_MAP_DSCP_TO_TC:
    case NDI_QOS_MAP_DSCP_TO_COLOR:
    case NDI_QOS_MAP_TC_TO_DOT1P:
    case NDI_QOS_MAP_TC_TO_DSCP:
    case NDI_QOS_MAP_TC_COLOR_TO_DOT1P:
    case NDI_QOS_MAP_TC_COLOR_TO_DSCP:
    case NDI_QOS_MAP_TC_TO_QUEUE:
    case NDI_QOS_MAP_TC_TO_PG:
    case NDI_QOS_MAP_PFC_TO_QUEUE:
    case NDI_QOS_MAP_PG_TO_PFC:
        entries = &(this->map_info);
        break;

    default:
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                "Map entry type incorrect");
        return STD_ERR(QOS, CFG, 0);
    }


    map_entries_iter_t it = entries->find(key.any);
    if (it != entries->end()) {
        // treat it as overwrite
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "Map entry overwrite");

        nas_qos_map_entry & ent = it->second;
        //overwrite existing entry
        switch (this->get_type()) {
        case NDI_QOS_MAP_DOT1P_TO_TC_COLOR:
        case NDI_QOS_MAP_DSCP_TO_TC_COLOR:
            ent.set_value_tc(entry.get_value().tc);
            ent.set_value_color(entry.get_value().color);
            break;
        case NDI_QOS_MAP_DOT1P_TO_TC:
        case NDI_QOS_MAP_DSCP_TO_TC:
            ent.set_value_tc(entry.get_value().tc);
            break;
        case NDI_QOS_MAP_DOT1P_TO_COLOR:
        case NDI_QOS_MAP_DSCP_TO_COLOR:
            ent.set_value_color(entry.get_value().color);
            break;
        case NDI_QOS_MAP_TC_TO_DOT1P:
        case NDI_QOS_MAP_TC_COLOR_TO_DOT1P:
            ent.set_value_dot1p(entry.get_value().dot1p);
            break;
        case NDI_QOS_MAP_TC_TO_DSCP:
        case NDI_QOS_MAP_TC_COLOR_TO_DSCP:
            ent.set_value_dscp(entry.get_value().dscp);
            break;
        case NDI_QOS_MAP_TC_TO_QUEUE:
        case NDI_QOS_MAP_PFC_TO_QUEUE:
            ent.set_value_queue_num(entry.get_value().queue_num);
            break;
        case NDI_QOS_MAP_TC_TO_PG:
            ent.set_value_pg(entry.get_value().pg);
            break;
        case NDI_QOS_MAP_PG_TO_PFC:
            // PFC prio is encoded in the combinational key already
            break;
        default:
            break;
        }
        return STD_ERR_OK;
    }

    // insert the new entry
    entries->insert(std::make_pair(key.any, std::move(entry)));

    return STD_ERR_OK;
}

t_std_error nas_qos_map::del_map_entry(nas_qos_map_entry_key_t key)
{
    map_entries * entries = this->get_map_entries();

    if (entries == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "No map entry to delete");
        return STD_ERR(QOS, CFG, 0);
    }

    // delete the entry
    entries->erase(key.any);

    return STD_ERR_OK;
}

map_entries * nas_qos_map::get_map_entries()
{
    switch (this->get_type()) {
    case NDI_QOS_MAP_DOT1P_TO_TC_COLOR:
    case NDI_QOS_MAP_DOT1P_TO_TC:
    case NDI_QOS_MAP_DOT1P_TO_COLOR:
    case NDI_QOS_MAP_DSCP_TO_TC_COLOR:
    case NDI_QOS_MAP_DSCP_TO_TC:
    case NDI_QOS_MAP_DSCP_TO_COLOR:
    case NDI_QOS_MAP_TC_TO_DOT1P:
    case NDI_QOS_MAP_TC_TO_DSCP:
    case NDI_QOS_MAP_TC_COLOR_TO_DOT1P:
    case NDI_QOS_MAP_TC_COLOR_TO_DSCP:
    case NDI_QOS_MAP_TC_TO_QUEUE:
    case NDI_QOS_MAP_TC_TO_PG:
    case NDI_QOS_MAP_PG_TO_PFC:
    case NDI_QOS_MAP_PFC_TO_QUEUE:
        return &(this->map_info);
        break;
    default:
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                "Map entry type incorrect");
        return NULL;
    }

    return NULL;
}



