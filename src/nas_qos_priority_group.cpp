
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
 * \file   nas_qos_priority_group.cpp
 * \brief  NAS QOS priority_group Object
 * \date   05-2015
 * \author
 */

#include "event_log.h"
#include "std_assert.h"
#include "nas_qos_common.h"
#include "nas_qos_priority_group.h"
#include "dell-base-qos.h"
#include "nas_ndi_qos.h"
#include "nas_base_obj.h"
#include "nas_qos_switch.h"

nas_qos_priority_group::nas_qos_priority_group (nas_qos_switch* switch_p,
                                                nas_qos_priority_group_key_t key)
           : base_obj_t (switch_p),
             key(key)
{
    priority_group_id = NDI_QOS_NULL_OBJECT_ID;
    buffer_profile = NDI_QOS_NULL_OBJECT_ID;
    ndi_port_id = {0};
}

const nas_qos_switch& nas_qos_priority_group::get_switch()
{
    return static_cast<const nas_qos_switch&> (base_obj_t::get_switch());
}


void nas_qos_priority_group::commit_create (bool rolling_back)

{
    base_obj_t::commit_create(rolling_back);
}

void* nas_qos_priority_group::alloc_fill_ndi_obj (nas::mem_alloc_helper_t& m)
{
    // NAS Qos priority_group does not allocate memory to save the incoming tentative attributes
    return this;
}

bool nas_qos_priority_group::push_create_obj_to_npu (npu_id_t npu_id,
                                     void* ndi_obj)
{
    return true;
}


bool nas_qos_priority_group::push_delete_obj_to_npu (npu_id_t npu_id)
{
    return true;
}

bool nas_qos_priority_group::is_leaf_attr (nas_attr_id_t attr_id)
{
    // Table of function pointers to handle modify of Qos priority_group
    // attributes.
    static const std::unordered_map <BASE_QOS_PRIORITY_GROUP_t,
                                     bool,
                                     std::hash<int>>
        _leaf_attr_map =
    {
        // modifiable objects
        {BASE_QOS_PRIORITY_GROUP_BUFFER_PROFILE_ID,  true},

        //The NPU ID list attribute is handled by the base object itself.
    };

    return (_leaf_attr_map.at(static_cast<BASE_QOS_PRIORITY_GROUP_t>(attr_id)));
}

bool nas_qos_priority_group::push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                           npu_id_t npu_id)
{
    t_std_error rc = STD_ERR_OK;
    nas_qos_buffer_profile *p_buffer_profile;
    nas_qos_switch & nas_switch = const_cast<nas_qos_switch &>(get_switch());
    ndi_obj_id_t ndi_obj;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "Modifying npu: %d, attr_id %d",
                    npu_id, attr_id);

    switch (attr_id) {
    case BASE_QOS_PRIORITY_GROUP_BUFFER_PROFILE_ID:
        if (buffer_profile == 0LL) {
            ndi_obj = NDI_QOS_NULL_OBJECT_ID;
        } else {
            p_buffer_profile = nas_switch.get_buffer_profile(buffer_profile);
            if (p_buffer_profile == NULL) {
                EV_LOG_TRACE(ev_log_t_QOS, 3, "QOS", "buffer_profile id %u not found",
                                buffer_profile);
                rc = STD_ERR(QOS, CFG, 0);
                break;
            }

            ndi_obj = p_buffer_profile->ndi_obj_id(npu_id);
        }

        rc = ndi_qos_set_priority_group_buffer_profile_id(ndi_port_id,
                                       ndi_obj_id(),
                                       ndi_obj);

        break;

    default:
        STD_ASSERT (0); //non-modifiable object
    }

    if (rc != STD_ERR_OK) {
            throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                "NDI attribute Set Failed"};
    }

    return true;
}

