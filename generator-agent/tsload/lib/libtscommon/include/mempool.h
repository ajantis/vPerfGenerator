/*
 * mempool.h
 *
 *  Created on: Nov 28, 2012
 *      Author: myaut
 */

#ifndef MEMPOOL_H_
#define MEMPOOL_H_

#include <stdlib.h>

void* mp_malloc(size_t sz);
void* mp_realloc(void* old, size_t sz);
void mp_free(void* ptr);

int mempool_init(void);
void mempool_fini(void);

#endif /* MEMPOOL_H_ */
