/*
 * mempool.c
 *
 *  Created on: Nov 28, 2012
 *      Author: myaut
 */

#include <threads.h>
#include <mempool.h>

#include <libjson.h>

#include <stdlib.h>

thread_mutex_t mp_mutex;

/**
 * Memory pools
 *
 * Currently - only MT-safe implementation for malloc/realloc/free
 */

void* mp_malloc(size_t sz) {
	void* ptr;

	ptr = malloc(sz);

	return ptr;
}

void* mp_realloc(void* old, size_t sz) {
	void* ptr;

	ptr = realloc(old, sz);

	return ptr;
}

void mp_free(void* ptr) {
	free(ptr);
}

int mempool_init(void) {
	mutex_init(&mp_mutex, "mp_mutex");

	json_register_memory_callbacks(mp_malloc,
			mp_realloc, mp_free);

	return 0;
}

