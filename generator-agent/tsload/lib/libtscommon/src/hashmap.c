/*
 * hashmap.c
 *
 *  Created on: 25.11.2012
 *      Author: myaut
 */

#include <hashmap.h>
#include <threads.h>

#include <assert.h>

static inline void* hm_next(hashmap_t* hm, void* obj) {
	return *((void**) (obj + hm->hm_off_next));
}

static inline void hm_set_next(hashmap_t* hm, void* obj, void* next) {
	*((void**) (obj + hm->hm_off_next)) = next;
}

static inline void* hm_get_key(hashmap_t* hm, void* obj) {
	return (obj + hm->hm_off_key);
}

static inline unsigned hm_hash_object(hashmap_t* hm, void* obj) {
	return hm->hm_hash_key(hm_get_key(hm, obj));
}

void hash_map_init(hashmap_t* hm, const char* name) {
	int i = 0;

	for(i = 0; i < hm->hm_size; ++i) {
		hm->hm_heads[i] = NULL;
	}

	mutex_init(&hm->hm_mutex, name);
}

void hash_map_insert(hashmap_t* hm, void* object) {
	unsigned hash = hm_hash_object(hm, object);

	void** head = hm->hm_heads + hash;
	void* iter;
	void* next;

	mutex_lock(&hm->hm_mutex);
	if(*head == NULL) {
		*head = object;
	}
	else {
		iter = *head;
		next = hm_next(hm, iter);

		/*FIXME: duplicates*/

		while(next != NULL) {
			iter = next;
			next = hm_next(hm, iter);
		}

		hm_set_next(hm, iter, object);
	}
	mutex_unlock(&hm->hm_mutex);
}

void hash_map_remove(hashmap_t* hm, void* object) {
	unsigned hash = hm_hash_object(hm, object);

	void** head = hm->hm_heads + hash;
	void* iter;
	void* next;

	mutex_lock(&hm->hm_mutex);

	assert(*head != NULL);

	if(*head == object) {
		iter = hm_next(hm, *head);
		*head = iter;
	}
	else {
		iter = *head;
		next = hm_next(hm, iter);

		/*FIXME: not found*/

		while(next != object) {
			assert(next != NULL);

			iter = next;
			next = hm_next(hm, iter);
		}

		hm_set_next(hm, iter, hm_next(hm, object));
	}

	mutex_unlock(&hm->hm_mutex);
}

void* hash_map_find(hashmap_t* hm, void* key) {
	unsigned hash = hm->hm_hash_key(key);
	void* iter = hm->hm_heads[hash];

	mutex_lock(&hm->hm_mutex);

	while(iter != NULL) {
		if(hm->hm_compare(hm_get_key(hm, iter), key) == 0)
			break;

		iter =  hm_next(hm, iter);
	}

	mutex_unlock(&hm->hm_mutex);

	return iter;
}

void hash_map_walk(hashmap_t* hm, void (*func)(void* object)) {
	int i = 0;
	void* iter = NULL;

	mutex_lock(&hm->hm_mutex);
	for(i = 0; i < hm->hm_size; ++i) {
		iter = hm->hm_heads[i];

		while(iter != NULL) {
			func(iter);
			iter =  hm_next(hm, iter);
		}
	}
	mutex_unlock(&hm->hm_mutex);
}

void hash_map_destroy(hashmap_t* hm) {
	mutex_destroy(&hm->hm_mutex);
}
