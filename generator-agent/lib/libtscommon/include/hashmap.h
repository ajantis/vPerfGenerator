/*
 * hashmap.h
 *
 *  Created on: 25.11.2012
 *      Author: myaut
 */

#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <defs.h>
#include <threads.h>

#include <stddef.h>

/**
 * Hash maps
 * */

#ifdef _MSC_VER
typedef char hm_item_t;
typedef char hm_key_t;
#else
typedef void hm_item_t;
typedef void hm_key_t;
#endif

typedef struct {
	size_t			hm_size;
	hm_item_t** 	hm_heads;

	thread_mutex_t hm_mutex;

	ptrdiff_t hm_off_key;
	ptrdiff_t hm_off_next;

	unsigned (*hm_hash_key)(const hm_key_t* key);
	boolean_t (*hm_compare)(const hm_key_t* key1, const hm_key_t* key2);
} hashmap_t;

LIBEXPORT void hash_map_init(hashmap_t* hm, const char* name);
LIBEXPORT int  hash_map_insert(hashmap_t* hm, hm_item_t* object);
LIBEXPORT int  hash_map_remove(hashmap_t* hm, hm_item_t* object);
LIBEXPORT void* hash_map_find(hashmap_t* hm, const hm_key_t* key);
LIBEXPORT void* hash_map_walk(hashmap_t* hm, int (*func)(hm_item_t* object, void* arg), void* arg);
LIBEXPORT void hash_map_destroy(hashmap_t* hm);

LIBEXPORT unsigned hm_string_hash(const hm_key_t* str, unsigned mask);

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
	hm_compare_##name(const hm_key_t* key1, 		\
					  const hm_key_t* key2) 		\
	hm_compare_body								    \
												    \
	static unsigned									\
	hm_hash_##name(const hm_key_t* key)				\
	hm_hash_body									\
													\
	static type* hm_heads_##name[size];				\
	static hashmap_t name = {						\
		SM_INIT(.hm_size, size),							\
		SM_INIT(.hm_heads, (void**) hm_heads_##name),		\
		SM_INIT(.hm_mutex, THREAD_MUTEX_INITIALIZER),		\
		SM_INIT(.hm_off_key, offsetof(type, key_field)),	\
		SM_INIT(.hm_off_next, offsetof(type, next_field)),	\
		SM_INIT(.hm_hash_key, hm_hash_##name),				\
		SM_INIT(.hm_compare, hm_compare_##name)				\
	};

#define HASH_MAP_OK				0
#define HASH_MAP_DUPLICATE		-1
#define HASH_MAP_NOT_FOUND		-2

#define HM_WALKER_CONTINUE		0
#define HM_WALKER_STOP			0x01
#define HM_WALKER_REMOVE		0x02

#ifndef NO_JSON
#include <libjson.h>

typedef JSONNODE* (*hm_json_formatter)(hm_item_t* object);

struct hm_fmt_context {
	JSONNODE* node;
	hm_json_formatter formatter;
};

LIBEXPORT JSONNODE* json_hm_format_bykey(hashmap_t* hm, hm_json_formatter formatter, void* key);
LIBEXPORT JSONNODE* json_hm_format_all(hashmap_t* hm, hm_json_formatter formatter);

#endif

#endif
