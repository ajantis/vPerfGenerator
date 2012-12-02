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

	ptrdiff_t hm_off_key;
	ptrdiff_t hm_off_next;

	unsigned (*hm_hash_key)(void* key);
	int (*hm_compare)(void* key1, void* key2);
} hashmap_t;

void hash_map_init(hashmap_t* hm, const char* name);
void  hash_map_insert(hashmap_t* hm, void* object);
void  hash_map_remove(hashmap_t* hm, void* object);
void* hash_map_find(hashmap_t* hm, void* key);
void hash_map_walk(hashmap_t* hm, void (*func)(void* object));

#define DECLARE_HASH_MAP(name, type, size, key_field, next_field, hm_hash_body, hm_compare_body) \
	static int 										\
	hm_compare_##name(void* key1, void* key2) 		\
	hm_compare_body								    \
												    \
	static unsigned									\
	hm_hash_##name(void* key)						\
	hm_hash_body									\
													\
	static type* hm_heads_##name[size];				\
	static hashmap_t name = {						\
		.hm_size = size,							\
		.hm_heads = (void**) hm_heads_##name,		\
		.hm_off_key = offsetof(type, key_field),	\
		.hm_off_next = offsetof(type, next_field),	\
		.hm_hash_key = hm_hash_##name,				\
		.hm_compare = hm_compare_##name,			\
	};

#endif
