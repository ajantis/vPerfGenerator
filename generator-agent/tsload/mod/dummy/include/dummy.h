/*
 * dummy.h
 *
 *  Created on: 08.11.2012
 *      Author: myaut
 */

#ifndef DUMMY_H_
#define DUMMY_H_

#include <wlparam.h>

enum dummy_test {
	DUMMY_READ,
	DUMMY_WRITE
};

struct dummy_workload {
	wlp_size_t file_size;
	wlp_size_t block_size;

	wlp_string_t path[512];
	wlp_strset_t test;

	wlp_bool_t	 sparse;

	int	fd;

	void* block;
};

#endif /* DUMMY_H_ */
