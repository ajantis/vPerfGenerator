/*
 * hashmap.h
 *
 *  Created on: 25.11.2012
 *      Author: myaut
 */

#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <stddef.h>
#include <threads.h>

typedef struct {
	size_t	hm_size;
	void** 	hm_heads;

	thread_mutex_t hm_mutex;

	unsigned (*hm_hash_object)(void* obj);
	unsigned (*hm_hash_key)(void* key);

	void* (*hm_next)(void* obj);
	void (*hm_set_next)(void* obj, void* next);

	int (*hm_compare)(void* obj, void* key);
} hashmap_t;

void hash_map_init(hashmap_t* hm, const char* name);
void  hash_map_insert(hashmap_t* hm, void* object);
void  hash_map_remove(hashmap_t* hm, void* object);
void* hash_map_find(hashmap_t* hm, void* key);
void hash_map_walk(hashmap_t* hm, void (*func)(void* object));

#endif
