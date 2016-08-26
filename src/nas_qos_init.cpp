
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
 * \file   nas_qos_init.cpp
 * \brief  NAS QOS Entry Point
 * \date   02-2015
 * \author
 */

#include "event_log.h"
#include "std_error_codes.h"
#include "nas_qos_common.h"
#include "nas_qos_switch_list.h"
#include "nas_qos_cps.h"
#include "dell-base-qos.h"
#include "nas_qos_init.h"
#include "nas_qos_queue.h"

static t_std_error cps_init ()
{
    cps_api_operation_handle_t       h;
    cps_api_return_code_t            cps_rc;
    cps_api_registration_functions_t f;

    if ((cps_rc = cps_api_operation_subsystem_init(&h,1))
          != cps_api_ret_code_OK) {
        return STD_ERR(QOS, FAIL, cps_rc);
    }

    memset(&f,0,sizeof(f));

    f.handle = h;
    f._read_function = nas_qos_cps_api_read;
    f._write_function = nas_qos_cps_api_write;
    f._rollback_function = nas_qos_cps_api_rollback;

    /* Register all QoS object */
    cps_api_key_init(&f.key,
                     cps_api_qualifier_TARGET,
                     (cps_api_object_category_types_t)cps_api_obj_CAT_BASE_QOS,
                     0, /* register all sub-categories */
                     0);

    if ((cps_rc = cps_api_register(&f)) != cps_api_ret_code_OK) {
        return STD_ERR(QOS, FAIL, cps_rc);
    }

    return STD_ERR_OK;
}

extern "C" {
/**
 * This function initializes the lower NAS related QoS data structure
 * @Return   Standard Error Code
 */
t_std_error nas_qos_init(void)
{
    t_std_error         rc = STD_ERR_OK;

    EV_LOG_TRACE(ev_log_t_QOS, 3, "NAS-QOS", "Initializing NAS-QOS data structures");

    do {

        if ((rc = cps_init ()) != STD_ERR_OK) {
            break;
        }

    } while (0);

    return rc;
}


}
