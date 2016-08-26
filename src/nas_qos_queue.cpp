
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
 * \file   nas_qos_queue.cpp
 * \brief  NAS QOS queue Object
 * \date   02-2015
 * \author
 */

#include "event_log.h"
#include "std_assert.h"
#include "nas_qos_common.h"
#include "nas_qos_queue.h"
#include "dell-base-qos.h"
#include "nas_ndi_qos.h"
#include "nas_base_obj.h"

#include "nas_qos_switch.h"

nas_qos_queue::nas_qos_queue (nas_qos_switch* switch_p,
                              nas_qos_queue_key_t key)
           : base_obj_t (switch_p),
             key(key)
{
    parent_id = NDI_QOS_NULL_OBJECT_ID;
    queue_id = 0;
    wred_id = 0;
    buffer_profile = 0;
    scheduler_profile = 0;
    ndi_port_id = {0};
}

const nas_qos_switch& nas_qos_queue::get_switch()
{
    return static_cast<const nas_qos_switch&> (base_obj_t::get_switch());
}

bool  nas_qos_queue::opaque_data_to_cps (cps_api_object_t cps_obj) const
{
    constexpr size_t  attr_size = 1;
    cps_api_attr_id_t  attr_id_list[] = {BASE_QOS_QUEUE_DATA};
    return nas::ndi_obj_id_table_cps_serialize (_ndi_obj_ids, cps_obj, attr_id_list, attr_size);
}

void nas_qos_queue::commit_create (bool rolling_back)

{
}

void* nas_qos_queue::alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m)
{
    // NAS Qos queue does not allocate memory to save the incoming tentative attributes
    return this;
}

bool nas_qos_queue::push_create_obj_to_npu (npu_id_t npu_id,
                                     void* ndi_obj)
{
    return true;
}


bool nas_qos_queue::push_delete_obj_to_npu (npu_id_t npu_id)
{
    return true;
}

bool nas_qos_queue::is_leaf_attr (nas_attr_id_t attr_id)
{
    // Table of function pointers to handle modify of Qos queue
    // attributes.
    static const std::unordered_map <BASE_QOS_QUEUE_t,
                                     bool,
                                     std::hash<int>>
        _leaf_attr_map =
    {
        // modifiable objects
        {BASE_QOS_QUEUE_WRED_ID,                true},
        {BASE_QOS_QUEUE_BUFFER_PROFILE_ID ,     true},
        {BASE_QOS_QUEUE_SCHEDULER_PROFILE_ID ,  true},
    };

    return (_leaf_attr_map.at(static_cast<BASE_QOS_QUEUE_t>(attr_id)));
}

bool nas_qos_queue::push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                           npu_id_t npu_id)
{
    nas_qos_scheduler *p_sched;
    nas_qos_wred *p_wred;
    nas_qos_buffer_profile *p_buf_prof;

    t_std_error rc = STD_ERR_OK;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Modifying npu: %d, attr_id %d",
                    npu_id, attr_id);

    ndi_port_t ndi_port_id = get_ndi_port_id();

    nas_qos_switch & nas_switch = const_cast<nas_qos_switch &>(get_switch());
    ndi_obj_id_t ndi_obj;

    switch (attr_id) {
    case BASE_QOS_QUEUE_WRED_ID:
        if (wred_id == 0LL) {
            ndi_obj = NDI_QOS_NULL_OBJECT_ID;
        } else {
            p_wred = nas_switch.get_wred(wred_id);
            if (p_wred == NULL) {
                EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "wred_profile id %u not found",
                                wred_id);
                rc = STD_ERR(QOS, CFG, 0);
                break;
            }

            ndi_obj = p_wred->ndi_obj_id(npu_id);
        }
        rc = ndi_qos_set_queue_wred_id(ndi_port_id,
                                    ndi_obj_id(),
                                    ndi_obj);
        break;

    case BASE_QOS_QUEUE_BUFFER_PROFILE_ID:
        if (buffer_profile == 0LL) {
            ndi_obj = NDI_QOS_NULL_OBJECT_ID;
        } else {
            p_buf_prof = nas_switch.get_buffer_profile(buffer_profile);
            if (p_buf_prof == NULL) {
                EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "buffer_profile id %u not found",
                                buffer_profile);
                rc = STD_ERR(QOS, CFG, 0);
                break;
            }

            ndi_obj = p_buf_prof->ndi_obj_id(npu_id);
        }
        rc = ndi_qos_set_queue_buffer_profile_id(ndi_port_id,
                                            ndi_obj_id(),
                                            ndi_obj);
        break;

    case BASE_QOS_QUEUE_SCHEDULER_PROFILE_ID:
        if (scheduler_profile == 0LL) {
            ndi_obj = NDI_QOS_NULL_OBJECT_ID;
        } else {
            p_sched = nas_switch.get_scheduler(scheduler_profile);
            if (p_sched == NULL) {
                EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "scheduler_profile id %u not found",
                                scheduler_profile);
                rc = STD_ERR(QOS, CFG, 0);
                break;
            }
            ndi_obj = p_sched->ndi_obj_id(npu_id);
        }
        rc = ndi_qos_set_queue_scheduler_profile_id(ndi_port_id,
                                            ndi_obj_id(),
                                            ndi_obj);

        break;

    default:
        STD_ASSERT (0); //non-modifiable object
    }

    if (rc != STD_ERR_OK)
    {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
            "NDI attribute Set Failed"};
    }

   return true;
}

