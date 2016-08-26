
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
 * \file   nas_qos_map_entry.cpp
 * \brief  NAS QOS dot1p|dscp to tc/color map Object
 */

#include "event_log.h"
#include "std_assert.h"
#include "nas_qos_common.h"
#include "nas_qos_map.h"
#include "nas_qos_map_entry.h"
#include "dell-base-qos.h"
#include "nas_ndi_qos.h"
#include "nas_base_obj.h"
#include "nas_qos_switch.h"

nas_qos_map_entry::nas_qos_map_entry (nas_qos_switch* switch_p,
        nas_obj_id_t map_id,
        nas_qos_map_type_t type,
        nas_qos_map_entry_key_t key)
           : base_obj_t (switch_p), map_id(map_id), type(type), key(key)
{
    value = {0};
}

const nas_qos_switch& nas_qos_map_entry::get_switch()
{
    return static_cast<const nas_qos_switch&> (base_obj_t::get_switch());
}

void nas_qos_map_entry::commit_create (bool rolling_back)

{
    switch (type) {
    case NDI_QOS_MAP_DOT1P_TO_TC_COLOR:
        if (!is_attr_dirty (BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_TC)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute TC not present"};
        }

        if (!is_attr_dirty (BASE_QOS_DOT1P_TO_TC_COLOR_MAP_ENTRY_COLOR)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute COLOR not present"};
        }
        break;

    case NDI_QOS_MAP_DOT1P_TO_TC:
        if (!is_attr_dirty (BASE_QOS_DOT1P_TO_TC_MAP_ENTRY_TC)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute TC not present"};
        }
        break;

    case NDI_QOS_MAP_DOT1P_TO_COLOR:
        if (!is_attr_dirty (BASE_QOS_DOT1P_TO_COLOR_MAP_ENTRY_COLOR)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute COLOR not present"};
        }
        break;

    case NDI_QOS_MAP_DSCP_TO_TC_COLOR:
        if (!is_attr_dirty (BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_TC)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute TC not present"};
        }

        if (!is_attr_dirty (BASE_QOS_DSCP_TO_TC_COLOR_MAP_ENTRY_COLOR)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute COLOR not present"};
        }
        break;

    case NDI_QOS_MAP_DSCP_TO_TC:
        if (!is_attr_dirty (BASE_QOS_DSCP_TO_TC_MAP_ENTRY_TC)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute TC not present"};
        }
        break;

    case NDI_QOS_MAP_DSCP_TO_COLOR:
        if (!is_attr_dirty (BASE_QOS_DSCP_TO_COLOR_MAP_ENTRY_COLOR)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute COLOR not present"};
        }
        break;

    case NDI_QOS_MAP_TC_TO_DOT1P:
        if (!is_attr_dirty (BASE_QOS_TC_TO_DOT1P_MAP_ENTRY_DOT1P)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute Dot1p not present"};
        }
        break;

    case NDI_QOS_MAP_TC_TO_DSCP:
        if (!is_attr_dirty (BASE_QOS_TC_TO_DSCP_MAP_ENTRY_DSCP)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute DSCP not present"};
        }
        break;

    case NDI_QOS_MAP_TC_COLOR_TO_DOT1P:
        if (!is_attr_dirty (BASE_QOS_TC_COLOR_TO_DOT1P_MAP_ENTRY_DOT1P)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute Dot1p not present"};
        }
        break;

    case NDI_QOS_MAP_TC_COLOR_TO_DSCP:
        if (!is_attr_dirty (BASE_QOS_TC_COLOR_TO_DSCP_MAP_ENTRY_DSCP)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute DSCP not present"};
        }
        break;

    case NDI_QOS_MAP_TC_TO_QUEUE:
        if (!is_attr_dirty (BASE_QOS_TC_TO_QUEUE_MAP_ENTRY_QUEUE_NUMBER)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute queue number not present"};
        }
        break;

    case NDI_QOS_MAP_TC_TO_PG:
        if (!is_attr_dirty (BASE_QOS_TC_TO_PRIORITY_GROUP_MAP_ENTRY_PRIORITY_GROUP)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute priority group not present"};
        }
        break;

    case NDI_QOS_MAP_PG_TO_PFC:
        // no non-key attribute
        break;

    case NDI_QOS_MAP_PFC_TO_QUEUE:
        if (!is_attr_dirty (BASE_QOS_PFC_PRIORITY_TO_QUEUE_MAP_ENTRY_QUEUE_NUMBER)) {
            throw nas::base_exception {NAS_BASE_E_CREATE_ONLY,
                            __PRETTY_FUNCTION__,
                            "Mandatory attribute queue number not present"};
        }
        break;

    default:
        break;
    }

    base_obj_t::commit_create(rolling_back);
}

bool nas_qos_map_entry::push_create_obj_to_npu (npu_id_t npu_id,
                                     void* ndi_obj)
{
    t_std_error rc;
    ndi_qos_map_struct_t s = {0};

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Creating Entry obj on NPU %d", npu_id);

    // Each mapping entry creation is converted into a set_qos_map

    s.key.dot1p = (uint8_t)(this->key.dot1p);
    s.key.dscp = (uint8_t)(this->key.dscp);
    s.key.tc = GET_KEY1(key);
    s.key.color = (BASE_QOS_PACKET_COLOR_t)GET_KEY2(key);

    if (this->get_type() == NDI_QOS_MAP_PG_TO_PFC) {
        s.key.pg = GET_KEY1(key);
        s.key.prio = GET_KEY2(key);
        s.value.prio = GET_KEY2(key);  // in case secondary key is retrieved as value by SAI
    }
    else if (this->get_type() == NDI_QOS_MAP_PFC_TO_QUEUE) {
        s.key.prio = GET_KEY1(key);
    }

    s.value.tc = this->value.tc;
    s.value.color = this->value.color;
    s.value.dot1p = this->value.dot1p;
    s.value.dscp = this->value.dscp;
    s.value.pg = this->value.pg;
    if (this->get_type() == NDI_QOS_MAP_TC_TO_QUEUE ||
        this->get_type() == NDI_QOS_MAP_PFC_TO_QUEUE) {
        if ((rc = get_ndi_qid(s.value.qid)) != STD_ERR_OK) {
            // should not be here!
            EV_LOG_ERR(ev_log_t_QOS, 3, "NAS-QOS", "FAILED to get NDI Qid");
            throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                "NDI QoS Map Entry Create Failed: NDI qid cannot be resolved"};
        }
    }

    // to all NPUs
    ndi_obj_id_t ndi_map_id;
    if ((rc = get_ndi_map_id(npu_id, &ndi_map_id)) != STD_ERR_OK) {
        EV_LOG_ERR(ev_log_t_QOS, 3, "NAS-QOS", "FAILED to get NDI map id");
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI QoS Map Entry Create Failed; NDI map not exist"};

    }

    if ((rc = ndi_qos_set_map_attr (npu_id,
                                  ndi_map_id,
                                  1, //one entry at a time
                                  &s))
            != STD_ERR_OK)
    {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI QoS Map Entry Create Failed"};
    }


    return true;

}


bool nas_qos_map_entry::push_delete_obj_to_npu (npu_id_t npu_id)
{
    t_std_error rc;
    ndi_qos_map_struct_t s = {0};

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Delete (Reset) Entry obj on NPU %d", npu_id);

    // Each mapping entry creation is converted into a set_qos_map

    s.key.dot1p = (uint8_t)(this->key.dot1p);
    s.key.dscp = (uint8_t)(this->key.dscp);
    s.key.tc = GET_KEY1(key);
    s.key.color = (BASE_QOS_PACKET_COLOR_t)GET_KEY2(key);
    if (this->get_type() == NDI_QOS_MAP_PG_TO_PFC) {
        s.key.pg = GET_KEY1(key);
        s.key.prio = GET_KEY2(key);
        s.value.prio = GET_KEY2(key);  // in case secondary key is retrieved as value by SAI
    }
    else if (this->get_type() == NDI_QOS_MAP_PFC_TO_QUEUE) {
        s.key.prio = GET_KEY1(key);
    }

    s.value.tc = NDI_DEFAULT_TRAFFIC_CLASS;
    s.value.color = NDI_DEFAULT_COLOR;
    s.value.dot1p = NDI_DEFAULT_DOT1P;
    s.value.dscp = NDI_DEFAULT_DSCP;
    s.value.pg = NDI_DEFAULT_PG;
    if (this->get_type() == NDI_QOS_MAP_TC_TO_QUEUE ||
        this->get_type() == NDI_QOS_MAP_PFC_TO_QUEUE) {
        if ((rc = get_ndi_qid(s.value.qid)) != STD_ERR_OK) {
            // should not be here!
            EV_LOG_ERR(ev_log_t_QOS, 3, "NAS-QOS", "FAILED to get NDI Qid");
            throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                "NDI QoS Map Entry Create Failed: NDI qid cannot be resolved"};
        }
    }

    ndi_obj_id_t ndi_map_id;
    if ((rc = get_ndi_map_id(npu_id, &ndi_map_id)) != STD_ERR_OK) {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI QoS Map Entry Delete Failed; NDI map not exist"};

    }

    if ((rc = ndi_qos_set_map_attr (npu_id,
                                  ndi_map_id,
                                  1, //one entry
                                  &s))
            != STD_ERR_OK)
    {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI QoS Map Entry delete Failed"};
    }

    return true;
}





bool nas_qos_map_entry::push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                           npu_id_t npu_id)
{
    // use the same entry-create api to overwrite existing entry.
    return push_create_obj_to_npu(npu_id, NULL);
}


// For TC-to-QUEUE mapping only
t_std_error nas_qos_map_entry::get_ndi_qid(ndi_obj_id_t &ndi_qid)
{
    BASE_QOS_QUEUE_TYPE_t  queue_type = (BASE_QOS_QUEUE_TYPE_t)GET_KEY2(key);

    switch (queue_type) {
    case BASE_QOS_QUEUE_TYPE_NONE:
        ndi_qid = get_value().queue_num;
        break;

    case BASE_QOS_QUEUE_TYPE_UCAST:
        ndi_qid = get_value().queue_num;
        break;

    case BASE_QOS_QUEUE_TYPE_MULTICAST:
    {   nas_qos_switch & switch_p = const_cast <nas_qos_switch &> (get_switch());
        ndi_qid = (get_value().queue_num + switch_p.ucast_queues_per_port);
    }
    break;

    default:
        return STD_ERR(QOS, CFG, 0);
    }
    return STD_ERR_OK;
}

t_std_error  nas_qos_map_entry::get_ndi_map_id(npu_id_t npu_id, ndi_obj_id_t * ndi_map_id)
{
    nas_qos_switch & switch_p = const_cast <nas_qos_switch &> (get_switch());

    try {
        if (switch_p.get_map(map_id) == NULL)
            return STD_ERR(QOS, CFG, 0);

        *ndi_map_id = switch_p.get_map(map_id)->ndi_obj_id(npu_id);
    }
    catch (...) {
        return STD_ERR(QOS, CFG, 0);
    }

    return STD_ERR_OK;
}

