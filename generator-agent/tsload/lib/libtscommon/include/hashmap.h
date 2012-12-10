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

/**
 * Hash maps
 * */

typedef struct {
	size_t	hm_size;
	void** 	hm_heads;

	thread_mutex_t hm_mutex;

	ptrdiff_t hm_off_key;
	ptrdiff_t hm_off_next;

	unsigned (*hm_hash_key)(const void* key);
	int (*hm_compare)(const void* key1, const void* key2);
} hashmap_t;

void hash_map_init(hashmap_t* hm, const char* name);
int  hash_map_insert(hashmap_t* hm, void* object);
int  hash_map_remove(hashmap_t* hm, void* object);
void* hash_map_find(hashmap_t* hm, const void* key);
void* hash_map_walk(hashmap_t* hm, int (*func)(void* object, void* arg), void* arg);
void hash_map_destroy(hashmap_t* hm);

/**
 * Declare hash map
 *
 * @param name name of hash map var (all corresponding objects will be named hm_(object)_(name))
 * @param type type of elements
 * @param size size of hash map (not of type)
 * @param key_field field of element that contains key
 * @param next_field field of element that contains pointer to next object
 * @param hm_hash_body function body {} that hashes key and returns hash (params: const void* key)
 * @param hm_compare_body function body {} that compares to keys (params: const void* key1, const void* key2)
 */
#define DECLARE_HASH_MAP(name, type, size, key_field, next_field, hm_hash_body, hm_compare_body) \
	static int 										\
	hm_compare_##name(const void* key1, 			\
					  const void* key2) 			\
	hm_compare_body								    \
												    \
	static unsigned									\
	hm_hash_##name(const void* key)					\
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

#define HASH_MAP_OK				0
#define HASH_MAP_DUPLICATE		-1
#define HASH_MAP_NOT_FOUND		-2

#define HM_WALKER_CONTINUE		0
#define HM_WALKER_STOP			1

#endif
