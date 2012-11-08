/*
 * dummy.h
 *
 *  Created on: 08.11.2012
 *      Author: myaut
 */

#ifndef DUMMY_H_
#define DUMMY_H_

struct dummy_workload {
	size_t file_size;
	size_t block_size;

	char path[512];
};

#endif /* DUMMY_H_ */
