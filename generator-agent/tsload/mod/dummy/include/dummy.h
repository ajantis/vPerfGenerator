/*
 * dummy.h
 *
 *  Created on: 08.11.2012
 *      Author: myaut
 */

#ifndef DUMMY_H_
#define DUMMY_H_

#include <wlparam.h>

struct dummy_workload {
	wlp_size_t file_size;
	wlp_size_t block_size;

	wlp_string_t path[512];
	wlp_string_t test[32];
};

#endif /* DUMMY_H_ */
