
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
 * \file   nas_qos_cps_queue.cpp
 * \brief  NAS qos queue related CPS API routines
 */

#include "cps_api_events.h"
#include "cps_api_operation.h"
#include "cps_api_key.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"

#include "event_log_types.h"
#include "event_log.h"
#include "std_error_codes.h"
#include "std_mutex_lock.h"

#include "nas_switch.h"
#include "hal_if_mapping.h"

#include "dell-base-qos.h"
#include "nas_qos_common.h"
#include "nas_qos_switch_list.h"
#include "nas_qos_cps.h"
#include "nas_qos_queue.h"
#include "nas_ndi_port.h"

#include <vector>

/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_queue_attr(cps_api_object_t obj,
                                              nas_qos_queue &queue);

static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                const nas::attr_set_t attr_set,
                                                const nas_qos_queue &queue);
static nas_qos_queue * nas_qos_cps_get_queue(uint32_t switch_id,
        nas_qos_queue_key_t key);
static cps_api_return_code_t nas_qos_cps_api_queue_set(
                                cps_api_object_t obj,
                                cps_api_object_t sav_obj);
t_std_error nas_qos_port_queue_init(hal_ifindex_t ifindex);

static std_mutex_lock_create_static_init_rec(queue_mutex);



/**
  * This function provides NAS-QoS queue CPS API write function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_queue_write(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&queue_mutex);

    switch (op) {
    case cps_api_oper_CREATE:
    case cps_api_oper_DELETE:
        // Queue requires no creation or deletion
        return NAS_QOS_E_UNSUPPORTED;

    case cps_api_oper_SET:
        return nas_qos_cps_api_queue_set(obj, param->prev);

    default:
        return NAS_QOS_E_UNSUPPORTED;
    }
}


static cps_api_return_code_t nas_qos_cps_get_queue_info(
                                cps_api_get_params_t * param,
                                uint32_t switch_id, uint_t port_id,
                                bool match_type, uint_t type,
                                bool match_q_num, uint_t q_num)
{
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NAS_QOS_E_FAIL;

    uint_t count = p_switch->get_number_of_port_queues(port_id);

    if  (count == 0) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "switch id %u, port id %u has no queues\n",
                switch_id, port_id);

        return NAS_QOS_E_FAIL;
    }

    std::vector<nas_qos_queue *> q_list(count);
    p_switch->get_port_queues(port_id, count, &q_list[0]);

    /* fill in data */
    cps_api_object_t ret_obj;

    for (uint_t i = 0; i < count; i++ ) {
        nas_qos_queue *queue = q_list[i];

        // filter out unwanted queues
        if (match_type && (queue->get_type() != type))
            continue;

        if (match_q_num && (queue->get_local_queue_id() != q_num))
            continue;


        ret_obj = cps_api_object_list_create_obj_and_append(param->list);
        if (ret_obj == NULL) {
            return cps_api_ret_code_ERR;
        }

        cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
                BASE_QOS_QUEUE_OBJ,
                cps_api_qualifier_TARGET);
        uint32_t val_port = queue->get_port_id();
        uint32_t val_type = queue->get_type();
        uint32_t val_queue_number = queue->get_local_queue_id();

        cps_api_set_key_data(ret_obj, BASE_QOS_QUEUE_PORT_ID,
                cps_api_object_ATTR_T_U32,
                &val_port, sizeof(uint32_t));
        cps_api_set_key_data(ret_obj, BASE_QOS_QUEUE_TYPE,
                cps_api_object_ATTR_T_U32,
                &val_type, sizeof(uint32_t));
        cps_api_set_key_data(ret_obj, BASE_QOS_QUEUE_QUEUE_NUMBER,
                cps_api_object_ATTR_T_U32,
                &val_queue_number, sizeof(uint32_t));

        cps_api_object_attr_add_u64(ret_obj, BASE_QOS_QUEUE_ID, queue->get_queue_id());

        // User configured objects
        if (queue->is_wred_id_set())
            cps_api_object_attr_add_u64(ret_obj, BASE_QOS_QUEUE_WRED_ID, queue->get_wred_id());
        if (queue->is_buffer_profile_set())
            cps_api_object_attr_add_u64(ret_obj, BASE_QOS_QUEUE_BUFFER_PROFILE_ID, queue->get_buffer_profile());
        if (queue->is_scheduler_profile_set())
            cps_api_object_attr_add_u64(ret_obj, BASE_QOS_QUEUE_SCHEDULER_PROFILE_ID, queue->get_scheduler_profile());

        queue->opaque_data_to_cps (ret_obj);
    }

    return cps_api_ret_code_OK;
}


/**
  * This function provides NAS-QoS queue CPS API read function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_queue_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{

    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t port_id_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_PORT_ID);
    cps_api_object_attr_t queue_type_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_TYPE);
    cps_api_object_attr_t queue_num_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_QUEUE_NUMBER);

    uint32_t switch_id = 0;

    if (port_id_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Port Id must be specified\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint_t port_id = cps_api_object_attr_data_u32(port_id_attr);

    uint_t queue_type;
    bool queue_type_specified = false;
    if (queue_type_attr) {
        queue_type_specified = true;
        queue_type = cps_api_object_attr_data_u32(queue_type_attr);
    }

    uint_t queue_num;
    bool queue_num_specified = false;
    if (queue_num_attr) {
        queue_num_specified = true;
        queue_num = cps_api_object_attr_data_u32(queue_num_attr);
    }

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Read switch id %u, port_id %u\n",
            switch_id, port_id);

    std_mutex_simple_lock_guard p_m(&queue_mutex);

    // If the port is not be initialized yet, initialize it in NAS
    nas_qos_port_queue_init(port_id);

    return nas_qos_cps_get_queue_info(param, switch_id, port_id,
                                queue_type_specified, queue_type,
                                queue_num_specified, queue_num);

}

/**
  * This function provides NAS-QoS queue CPS API rollback function
  * @Param      Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_queue_rollback(void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->prev,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    std_mutex_simple_lock_guard p_m(&queue_mutex);

    if (op == cps_api_oper_SET) {
        nas_qos_cps_api_queue_set(obj, NULL);
    }

    // create/delete are not allowed for queue, no roll-back is needed

    return cps_api_ret_code_OK;
}


static cps_api_return_code_t nas_qos_cps_api_queue_set(
                                cps_api_object_t obj,
                                cps_api_object_t sav_obj)
{
    cps_api_object_t tmp_obj;

    cps_api_object_attr_t port_id_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_PORT_ID);
    cps_api_object_attr_t queue_type_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_TYPE);
    cps_api_object_attr_t queue_num_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_QUEUE_NUMBER);

    if (port_id_attr == NULL ||
        queue_type_attr == NULL || queue_num_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Key incomplete in the message\n");
        return NAS_QOS_E_MISSING_KEY;
    }

    uint32_t switch_id = 0;
    uint_t port_id = cps_api_object_attr_data_u32(port_id_attr);
    uint_t queue_type = cps_api_object_attr_data_u32(queue_type_attr);
    uint_t queue_num = cps_api_object_attr_data_u32(queue_num_attr);

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
            "Modify switch id %u, port id %u, queue_type %u, queue_num %u \n",
            switch_id, port_id, queue_type, queue_num);

    // If the port is not be initialized yet, initialize it in NAS
    nas_qos_port_queue_init(port_id);

    nas_qos_queue_key_t key;
    key.port_id = port_id;
    key.type = (BASE_QOS_QUEUE_TYPE_t)queue_type;
    key.local_queue_id = queue_num;
    nas_qos_queue * queue_p = nas_qos_cps_get_queue(switch_id, key);
    if (queue_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                        "Queue not found in switch id %u\n",
                        switch_id);
        return NAS_QOS_E_FAIL;
    }

    /* make a local copy of the existing queue */
    nas_qos_queue queue(*queue_p);

    cps_api_return_code_t rc = cps_api_ret_code_OK;
    if ((rc = nas_qos_cps_parse_queue_attr(obj, queue)) != cps_api_ret_code_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Invalid information in the packet");
        return rc;
    }


    try {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Modifying switch id %u, port id %u queue info \n",
                switch_id, port_id);

        nas::attr_set_t modified_attr_list = queue.commit_modify(*queue_p, (sav_obj? false: true));


        // set attribute with full copy
        // save rollback info if caller requests it.
        // use modified attr list, current queue value
        if (sav_obj) {
            tmp_obj = cps_api_object_list_create_obj_and_append(sav_obj);
            if (!tmp_obj) {
                return cps_api_ret_code_ERR;
            }
            nas_qos_store_prev_attr(tmp_obj, modified_attr_list, *queue_p);
        }

        // update the local cache with newly set values
        *queue_p = queue;

    } catch (nas::base_exception e) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS queue Attr Modify error code: %d ",
                    e.err_code);
        return NAS_QOS_E_FAIL;

    } catch (...) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "NAS queue Modify Unexpected error code");
        return NAS_QOS_E_FAIL;
    }


    return cps_api_ret_code_OK;
}



/* Parse the attributes */
static cps_api_return_code_t  nas_qos_cps_parse_queue_attr(cps_api_object_t obj,
                                              nas_qos_queue &queue)
{
    uint64_t val64;
    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        switch (id) {
        case BASE_QOS_QUEUE_SWITCH_ID:
        case BASE_QOS_QUEUE_PORT_ID:
        case BASE_QOS_QUEUE_TYPE:
        case BASE_QOS_QUEUE_QUEUE_NUMBER:
        case BASE_QOS_QUEUE_ID:
            break; // These are not settable attr

        case BASE_QOS_QUEUE_WRED_ID:
            val64 = cps_api_object_attr_data_u64(it.attr);
            queue.set_wred_id((nas_obj_id_t)val64);
            break;


        case BASE_QOS_QUEUE_BUFFER_PROFILE_ID:
            val64 = cps_api_object_attr_data_u64(it.attr);
            queue.set_buffer_profile((nas_obj_id_t)val64);
            break;

        case BASE_QOS_QUEUE_SCHEDULER_PROFILE_ID:
            val64 = cps_api_object_attr_data_u64(it.attr);
            queue.set_scheduler_profile((nas_obj_id_t)val64);
            break;

        case CPS_API_ATTR_RESERVE_RANGE_END:
            // skip keys
            break;

        default:
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS", "Unrecognized option: %d", id);
            return NAS_QOS_E_UNSUPPORTED;
        }
    }

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t nas_qos_store_prev_attr(cps_api_object_t obj,
                                                    const nas::attr_set_t attr_set,
                                                    const nas_qos_queue &queue)
{
    // filling in the keys
    uint32_t switch_id = queue.switch_id();
    uint32_t val_port = queue.get_port_id();
    uint32_t val_type = queue.get_type();
    uint32_t val_queue_number = queue.get_local_queue_id();

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_QOS_QUEUE_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_SWITCH_ID,
            cps_api_object_ATTR_T_U32,
            &switch_id, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &val_port, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_TYPE,
            cps_api_object_ATTR_T_U32,
            &val_type, sizeof(uint32_t));
    cps_api_set_key_data(obj, BASE_QOS_QUEUE_QUEUE_NUMBER,
            cps_api_object_ATTR_T_U32,
            &val_queue_number, sizeof(uint32_t));

    for (auto attr_id: attr_set) {
        switch (attr_id) {
        case BASE_QOS_QUEUE_PORT_ID:
        case BASE_QOS_QUEUE_ID:
        case BASE_QOS_QUEUE_TYPE:
            break; // These are not settable attr

        case BASE_QOS_QUEUE_WRED_ID:
            cps_api_object_attr_add_u64(obj, attr_id,
                                        queue.get_wred_id());
            break;

        case BASE_QOS_QUEUE_BUFFER_PROFILE_ID:
            cps_api_object_attr_add_u64(obj, attr_id,
                                        queue.get_buffer_profile());
            break;

        case BASE_QOS_QUEUE_SCHEDULER_PROFILE_ID:
            cps_api_object_attr_add_u64(obj, attr_id,
                                        queue.get_scheduler_profile());
            break;

        default:
            break;
        }
    }

    return cps_api_ret_code_OK;
}

static nas_qos_queue * nas_qos_cps_get_queue(uint32_t switch_id,
        nas_qos_queue_key_t key)
{
    nas_qos_switch *p_switch = nas_qos_get_switch(switch_id);
    if (p_switch == NULL)
        return NULL;

    nas_qos_queue *queue_p = p_switch->get_queue(key);

    return queue_p;
}

// create per-port, per-queue instance
static t_std_error create_port_queue(hal_ifindex_t port_id,
                                    uint32_t local_queue_id,
                                    BASE_QOS_QUEUE_TYPE_t type,
                                    ndi_obj_id_t ndi_queue_id)
{
    interface_ctrl_t intf_ctrl;

    if (nas_qos_get_port_intf(port_id, &intf_ctrl) != true) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Cannot find NPU id for ifIndex: %d",
                      port_id);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_switch *switch_p = nas_qos_get_switch_by_npu(intf_ctrl.npu_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "switch_id of ifindex: %u cannot be found/created",
                     port_id);
        return NAS_QOS_E_FAIL;
    }

    try {
        // create the queue and add the queue to switch
        nas_obj_id_t queue_id = switch_p->alloc_queue_id();
        nas_qos_queue_key_t key;
        key.port_id = intf_ctrl.if_index;
        key.type = type;
        key.local_queue_id = local_queue_id;
        nas_qos_queue q (switch_p, key);

        q.set_queue_id(queue_id);
        q.add_npu(intf_ctrl.npu_id);
        q.set_ndi_port_id(intf_ctrl.npu_id, intf_ctrl.port_id);
        q.set_ndi_obj_id(ndi_queue_id);
        q.mark_ndi_created();

        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "NAS queue_id 0x%016lX is allocated for queue: type %u, local_qid %u, ndi_queue_id 0x%016lX",
                     queue_id, type, local_queue_id, ndi_queue_id);
       switch_p->add_queue(q);
    }
    catch (...) {
        return NAS_QOS_E_FAIL;
    }

    return STD_ERR_OK;

}

/* This function initializes the queues of a port
 * @Return standard error code
 */
t_std_error nas_qos_port_queue_init(hal_ifindex_t ifindex)
{
    interface_ctrl_t intf_ctrl;

    if (nas_qos_get_port_intf(ifindex, &intf_ctrl) == false) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Cannot find ifindex %u",
                     ifindex);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_switch *switch_p = nas_qos_get_switch_by_npu(intf_ctrl.npu_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "switch_id of npu_id: %u cannot be found/created",
                     intf_ctrl.npu_id);
        return NAS_QOS_E_FAIL;
    }

    if (switch_p->port_queue_is_initialized(ifindex))
        return STD_ERR_OK; // initialized

    ndi_port_t ndi_port_id;
    ndi_port_id.npu_id = intf_ctrl.npu_id;
    ndi_port_id.npu_port = intf_ctrl.port_id;

    /* get the number of queues per port */
    uint_t no_of_queue = ndi_qos_get_number_of_queues(ndi_port_id);
    if (no_of_queue == 0) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "No queues for npu_id %u, npu_port_id %u ",
                     ndi_port_id.npu_id, ndi_port_id.npu_port);
        return STD_ERR_OK;
    }

    /* get the list of ndi_queue id list */
    std::vector<ndi_obj_id_t> ndi_queue_id_list(no_of_queue);
    if (ndi_qos_get_queue_id_list(ndi_port_id, no_of_queue, &ndi_queue_id_list[0]) !=
            no_of_queue) {
        EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                     "Fail to retrieve all queues of npu_id %u, npu_port_id %u ",
                     ndi_port_id.npu_id, ndi_port_id.npu_port);
        return NAS_QOS_E_FAIL;
    }

    /* retrieve the ndi-queue type and assign the queues to with nas_queue_key */
    ndi_qos_queue_attribute_t ndi_qos_queue_attr;
    uint_t ucast_queue_id = 0;
    uint_t mcast_queue_id = 0;
    uint_t anycast_queue_id = 0;

    uint32_t local_queue_number = 0;
    BASE_QOS_QUEUE_TYPE_t type = BASE_QOS_QUEUE_TYPE_NONE;

    for (uint_t idx = 0; idx < no_of_queue; idx++) {

        ndi_qos_get_queue_attribute(ndi_port_id,
                                    ndi_queue_id_list[idx],
                                    &ndi_qos_queue_attr);

        if (ndi_qos_queue_attr.type == BASE_QOS_QUEUE_TYPE_UCAST) {
            local_queue_number = ucast_queue_id;
            type = BASE_QOS_QUEUE_TYPE_UCAST;

            ucast_queue_id++;
        }
        else if (ndi_qos_queue_attr.type == BASE_QOS_QUEUE_TYPE_MULTICAST) {
            local_queue_number = mcast_queue_id;
            type = BASE_QOS_QUEUE_TYPE_MULTICAST;

            mcast_queue_id++;
        }
        else if (ndi_qos_queue_attr.type == BASE_QOS_QUEUE_TYPE_NONE) {
            local_queue_number = anycast_queue_id;
            type = BASE_QOS_QUEUE_TYPE_NONE;

            anycast_queue_id++;
        }
        else {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                    "Unknown queue type: %u", ndi_qos_queue_attr.type);
            continue;
        }

        // Internally create NAS queue nodes and add to NAS QOS
        if (create_port_queue(ifindex, local_queue_number, type, ndi_queue_id_list[idx]) != STD_ERR_OK) {
            EV_LOG_TRACE(ev_log_t_QOS, ev_log_s_MAJOR, "QOS",
                         "Not able to create ifindex %u, local_queue_number %u, type %u",
                         ifindex, local_queue_number, type);
            return NAS_QOS_E_FAIL;
        }

    }

    return STD_ERR_OK;

}

/* Debugging and unit testing */
void dump_nas_qos_queues(nas_switch_id_t switch_id)
{
    nas_qos_switch * p_switch = nas_qos_get_switch(switch_id);

    if (p_switch == NULL)
        return;

    p_switch->dump_all_queues();

}

static uint64_t _get_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->packets ;
}
static uint64_t _get_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->bytes ;
}
static uint64_t _get_dropped_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->dropped_packets ;
}
static uint64_t _get_dropped_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->dropped_bytes ;
}
static uint64_t _get_green_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->green_packets ;
}
static uint64_t _get_green_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->green_bytes ;
}
static uint64_t _get_green_dropped_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->green_dropped_packets ;
}
static uint64_t _get_green_dropped_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->green_dropped_bytes ;
}
static uint64_t _get_yellow_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->yellow_packets ;
}
static uint64_t _get_yellow_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->yellow_bytes ;
}
static uint64_t _get_yellow_dropped_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->yellow_dropped_packets ;
}
static uint64_t _get_yellow_dropped_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->yellow_dropped_bytes ;
}
static uint64_t _get_red_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->red_packets ;
}
static uint64_t _get_red_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->red_bytes ;
}
static uint64_t _get_red_dropped_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->red_dropped_packets ;
}
static uint64_t _get_red_dropped_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->red_dropped_bytes ;
}
static uint64_t _get_green_discard_dropped_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->green_discard_dropped_packets ;
}
static uint64_t _get_green_discard_dropped_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->green_discard_dropped_bytes ;
}
static uint64_t _get_yellow_discard_dropped_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->yellow_discard_dropped_packets ;
}
static uint64_t _get_yellow_discard_dropped_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->yellow_discard_dropped_bytes ;
}
static uint64_t _get_red_discard_dropped_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->red_discard_dropped_packets ;
}
static uint64_t _get_red_discard_dropped_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->red_discard_dropped_bytes ;
}
static uint64_t _get_discard_dropped_packets (const nas_qos_queue_stat_counter_t *p) {
    return p->discard_dropped_packets ;
}
static uint64_t _get_discard_dropped_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->discard_dropped_bytes ;
}
static uint64_t _get_current_occupancy_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->current_occupancy_bytes ;
}
static uint64_t _get_watermark_bytes (const nas_qos_queue_stat_counter_t *p) {
    return p->watermark_bytes ;
}

typedef uint64_t (*_stat_get_fn) (const nas_qos_queue_stat_counter_t *p);

typedef struct _stat_attr_list_t {
    bool            read_ok;
    bool            write_ok;
    _stat_get_fn    fn;
}_stat_attr_list;

static const  std::unordered_map<nas_attr_id_t, _stat_attr_list, std::hash<int>>
    _queue_stat_attr_map = {
        {BASE_QOS_QUEUE_STAT_PACKETS,
                {true, true, _get_packets}},
        {BASE_QOS_QUEUE_STAT_BYTES,
                {true, true, _get_bytes}},
        {BASE_QOS_QUEUE_STAT_DROPPED_PACKETS,
                {true, true, _get_dropped_packets}},
        {BASE_QOS_QUEUE_STAT_DROPPED_BYTES,
                {true, true, _get_dropped_bytes}},
        {BASE_QOS_QUEUE_STAT_GREEN_PACKETS,
                {true, true, _get_green_packets}},
        {BASE_QOS_QUEUE_STAT_GREEN_BYTES,
                {true, true, _get_green_bytes}},
        {BASE_QOS_QUEUE_STAT_GREEN_DROPPED_PACKETS,
                {true, true, _get_green_dropped_packets}},
        {BASE_QOS_QUEUE_STAT_GREEN_DROPPED_BYTES,
                {true, true, _get_green_dropped_bytes}},
        {BASE_QOS_QUEUE_STAT_YELLOW_PACKETS,
                {true, true, _get_yellow_packets}},
        {BASE_QOS_QUEUE_STAT_YELLOW_BYTES,
                {true, true, _get_yellow_bytes}},
        {BASE_QOS_QUEUE_STAT_YELLOW_DROPPED_PACKETS,
                {true, true, _get_yellow_dropped_packets}},
        {BASE_QOS_QUEUE_STAT_YELLOW_DROPPED_BYTES,
                {true, true, _get_yellow_dropped_bytes}},
        {BASE_QOS_QUEUE_STAT_RED_PACKETS,
                {true, true, _get_red_packets}},
        {BASE_QOS_QUEUE_STAT_RED_BYTES,
                {true, true, _get_red_bytes}},
        {BASE_QOS_QUEUE_STAT_RED_DROPPED_PACKETS,
                {true, true, _get_red_dropped_packets}},
        {BASE_QOS_QUEUE_STAT_RED_DROPPED_BYTES,
                {true, true, _get_red_dropped_bytes}},
        {BASE_QOS_QUEUE_STAT_GREEN_DISCARD_DROPPED_PACKETS,
                {true, true, _get_green_discard_dropped_packets}},
        {BASE_QOS_QUEUE_STAT_GREEN_DISCARD_DROPPED_BYTES,
                {true, true, _get_green_discard_dropped_bytes}},
        {BASE_QOS_QUEUE_STAT_YELLOW_DISCARD_DROPPED_PACKETS,
                {true, true, _get_yellow_discard_dropped_packets}},
        {BASE_QOS_QUEUE_STAT_YELLOW_DISCARD_DROPPED_BYTES,
                {true, true, _get_yellow_discard_dropped_bytes}},
        {BASE_QOS_QUEUE_STAT_RED_DISCARD_DROPPED_PACKETS,
                {true, true, _get_red_discard_dropped_packets}},
        {BASE_QOS_QUEUE_STAT_RED_DISCARD_DROPPED_BYTES,
                {true, true, _get_red_discard_dropped_bytes}},
        {BASE_QOS_QUEUE_STAT_DISCARD_DROPPED_PACKETS,
                {true, true, _get_discard_dropped_packets}},
        {BASE_QOS_QUEUE_STAT_DISCARD_DROPPED_BYTES,
                {true, true, _get_discard_dropped_bytes}},
        {BASE_QOS_QUEUE_STAT_CURRENT_OCCUPANCY_BYTES,
                {true, false, _get_current_occupancy_bytes}},
        {BASE_QOS_QUEUE_STAT_WATERMARK_BYTES,
                {true, true, _get_watermark_bytes}},
       };


static uint64_t get_stats_by_type(const nas_qos_queue_stat_counter_t *p,
                                BASE_QOS_QUEUE_STAT_t id)
{
    try {
        return _queue_stat_attr_map.at(id).fn(p);
    } catch (std::out_of_range e) {
        return 0;
    }
}


/**
  * This function provides NAS-QoS queue stats CPS API read function
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_queue_stat_read (void * context,
                                            cps_api_get_params_t * param,
                                            size_t ix)
{

    cps_api_object_t obj = cps_api_object_list_get(param->filters, ix);
    cps_api_object_attr_t port_id_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_STAT_PORT_ID);
    cps_api_object_attr_t queue_type_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_STAT_TYPE);
    cps_api_object_attr_t queue_num_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_STAT_QUEUE_NUMBER);

    if (port_id_attr == NULL ||
        queue_type_attr == NULL || queue_num_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "Incomplete key: port-id, queue type and queue number must be specified\n");
        return NAS_QOS_E_MISSING_KEY;
    }
    uint32_t switch_id = 0;
    nas_qos_queue_key_t key;
    key.port_id = cps_api_object_attr_data_u32(port_id_attr);
    key.type = (BASE_QOS_QUEUE_TYPE_t)cps_api_object_attr_data_u32(queue_type_attr);
    key.local_queue_id = cps_api_object_attr_data_u32(queue_num_attr);


    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
            "Read switch id %u, port_id %u queue type %u, queue_number %u stat\n",
            switch_id, key.port_id, key.type, key.local_queue_id);

    std_mutex_simple_lock_guard p_m(&queue_mutex);

    // If the port is not be initialized yet, initialize it in NAS
    nas_qos_port_queue_init(key.port_id);

    nas_qos_switch *switch_p = nas_qos_get_switch(switch_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "switch_id %u not found", switch_id);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_queue * queue_p = switch_p->get_queue(key);
    if (queue_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Queue not found");
        return NAS_QOS_E_FAIL;
    }

    nas_qos_queue_stat_counter_t stats = {0};
    std::vector<BASE_QOS_QUEUE_STAT_t> counter_ids;

    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        if (id == BASE_QOS_QUEUE_STAT_SWITCH_ID ||
            id == BASE_QOS_QUEUE_STAT_PORT_ID ||
            id == BASE_QOS_QUEUE_STAT_TYPE ||
            id == BASE_QOS_QUEUE_STAT_QUEUE_NUMBER)
            continue; //key

        try {
            if (_queue_stat_attr_map.at(id).read_ok) {
                counter_ids.push_back((BASE_QOS_QUEUE_STAT_t)id);
            }
        } catch (std::out_of_range e) {
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Unknown queue STAT flag: %u, ignored", id);
        }
    }

    if (ndi_qos_get_queue_stats(queue_p->get_ndi_port_id(),
                                queue_p->ndi_obj_id(),
                                &counter_ids[0],
                                counter_ids.size(),
                                &stats) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Queue stats get failed");
        return NAS_QOS_E_FAIL;
    }

    // return stats objects to cps-app
    cps_api_object_t ret_obj = cps_api_object_list_create_obj_and_append(param->list);
    if (ret_obj == NULL) {
        return cps_api_ret_code_ERR;
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(ret_obj),
            BASE_QOS_QUEUE_STAT_OBJ,
            cps_api_qualifier_TARGET);
    cps_api_set_key_data(ret_obj, BASE_QOS_QUEUE_STAT_SWITCH_ID,
            cps_api_object_ATTR_T_U32,
            &switch_id, sizeof(uint32_t));
    cps_api_set_key_data(ret_obj, BASE_QOS_QUEUE_STAT_PORT_ID,
            cps_api_object_ATTR_T_U32,
            &(key.port_id), sizeof(uint32_t));
    cps_api_set_key_data(ret_obj, BASE_QOS_QUEUE_STAT_TYPE,
            cps_api_object_ATTR_T_U32,
            &(key.type), sizeof(uint32_t));
    cps_api_set_key_data(ret_obj, BASE_QOS_QUEUE_STAT_QUEUE_NUMBER,
            cps_api_object_ATTR_T_U32,
            &(key.local_queue_id), sizeof(uint32_t));

    uint64_t val64;
    for (uint_t i=0; i< counter_ids.size(); i++) {
        BASE_QOS_QUEUE_STAT_t id = counter_ids[i];
        val64 = get_stats_by_type(&stats, id);
        cps_api_object_attr_add_u64(ret_obj, id, val64);
    }

    return  cps_api_ret_code_OK;

}


/**
  * This function provides NAS-QoS queue stats CPS API clear function
  * User can use this function to clear the queue stats by setting relevant counters to zero
  * @Param    Standard CPS API params
  * @Return   Standard Error Code
  */
cps_api_return_code_t nas_qos_cps_api_queue_stat_clear (void * context,
                                            cps_api_transaction_params_t * param,
                                            size_t ix)
{
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    if (op != cps_api_oper_SET)
        return NAS_QOS_E_UNSUPPORTED;

    cps_api_object_attr_t port_id_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_STAT_PORT_ID);
    cps_api_object_attr_t queue_type_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_STAT_TYPE);
    cps_api_object_attr_t queue_num_attr = cps_api_get_key_data(obj, BASE_QOS_QUEUE_STAT_QUEUE_NUMBER);

    if (port_id_attr == NULL ||
        queue_type_attr == NULL || queue_num_attr == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
                "Incomplete key: port-id, queue type and queue number must be specified\n");
        return NAS_QOS_E_MISSING_KEY;
    }
    uint32_t switch_id = 0;
    nas_qos_queue_key_t key;
    key.port_id = cps_api_object_attr_data_u32(port_id_attr);
    key.type = (BASE_QOS_QUEUE_TYPE_t)cps_api_object_attr_data_u32(queue_type_attr);
    key.local_queue_id = cps_api_object_attr_data_u32(queue_num_attr);


    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS",
            "Read switch id %u, port_id %u queue type %u, queue_number %u stat\n",
            switch_id, key.port_id, key.type, key.local_queue_id);

    std_mutex_simple_lock_guard p_m(&queue_mutex);

    // If the port is not be initialized yet, initialize it in NAS
    nas_qos_port_queue_init(key.port_id);

    nas_qos_switch *switch_p = nas_qos_get_switch(switch_id);
    if (switch_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "switch_id %u not found", switch_id);
        return NAS_QOS_E_FAIL;
    }

    nas_qos_queue * queue_p = switch_p->get_queue(key);
    if (queue_p == NULL) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Queue not found");
        return NAS_QOS_E_FAIL;
    }

    std::vector<BASE_QOS_QUEUE_STAT_t> counter_ids;

    cps_api_object_it_t it;
    cps_api_object_it_begin(obj,&it);
    for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) ) {
        cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);
        if (id == BASE_QOS_QUEUE_STAT_SWITCH_ID ||
            id == BASE_QOS_QUEUE_STAT_PORT_ID ||
            id == BASE_QOS_QUEUE_STAT_TYPE ||
            id == BASE_QOS_QUEUE_STAT_QUEUE_NUMBER)
            continue; //key

        try {
            if (_queue_stat_attr_map.at(id).write_ok) {
                counter_ids.push_back((BASE_QOS_QUEUE_STAT_t)id);
            }
        } catch (std::out_of_range e) {
            EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Unknown queue STAT flag: %u, ignored", id);
        }
    }

    if (ndi_qos_clear_queue_stats(queue_p->get_ndi_port_id(),
                                queue_p->ndi_obj_id(),
                                &counter_ids[0],
                                counter_ids.size()) != STD_ERR_OK) {
        EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Queue stats clear failed");
        return NAS_QOS_E_FAIL;
    }


    return  cps_api_ret_code_OK;

}
