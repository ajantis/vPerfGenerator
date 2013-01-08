/*
 * tsload.h
 *
 *  Created on: Dec 3, 2012
 *      Author: myaut
 */

#ifndef TSLOAD_H_
#define TSLOAD_H_

#include <defs.h>
#include <errcode.h>

#include <tsinit.h>
#include <tstime.h>

#include <libjson.h>

/* TSLoad callbacks*/
typedef void (*tsload_error_msg_func)(ts_errcode_t code, const char* format, ...);
typedef void (*tsload_workload_status_func)(const char* wl_name,
					 					    int status,
										    int progress,
										    const char* config_msg);
typedef void (*tsload_requests_report_func)(JSONNODE* j_rq_list);

LIBIMPORT tsload_error_msg_func tsload_error_msg;
LIBIMPORT tsload_workload_status_func tsload_workload_status;
LIBIMPORT tsload_requests_report_func tsload_requests_report;

/* TSLoad calls */
LIBEXPORT JSONNODE* tsload_get_modules_info(void);

LIBEXPORT void tsload_configure_workload(const char* wl_name, JSONNODE* wl_params);
LIBEXPORT int tsload_provide_step(const char* wl_name, long step_id, unsigned num_rqs);
LIBEXPORT void tsload_start_workload(const char* wl_name, ts_time_t start_time);

LIBEXPORT int tsload_init(struct subsystem* xsubsys, unsigned xs_count);
LIBEXPORT int tsload_start(const char* basename);

#endif /* TSLOAD_H_ */
