/*
 * workload.h
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#ifndef WORKLOAD_H_
#define WORKLOAD_H_

#include <threadpool.h>
#include <wlparam.h>

typedef struct workload {
	thread_pool_t*	w_tp;
} workload_t;

#endif /* WORKLOAD_H_ */
