/*
 * hashmap.c
 *
 *  Created on: 25.11.2012
 *      Author: myaut
 */

#include <hashmap.h>
#include <threads.h>

#include <assert.h>

void hash_map_init(hashmap_t* hm, const char* name) {
	int i = 0;

	for(i = 0; i < hm->hm_size; ++hm) {
		hm->hm_heads[i] = NULL;
	}

	mutex_init(&hm->hm_mutex, name);
}

void hash_map_insert(hashmap_t* hm, void* object) {
	unsigned hash = hm->hm_hash_object(object);

	void** head = hm->hm_heads + hash;
	void* iter;
	void* next;

	mutex_lock(&hm->hm_mutex);
	if(*head == NULL) {
		*head = object;
	}
	else {
		iter = *head;
		next = hm->hm_next(iter);

		while(next != NULL) {
			iter = next;
			next = hm->hm_next(iter);
		}

		hm->hm_set_next(iter, object);
	}
	mutex_unlock(&hm->hm_mutex);
}

void hash_map_remove(hashmap_t* hm, void* object) {
	unsigned hash = hm->hm_hash_object(object);

	void** head = hm->hm_heads + hash;
	void* iter;
	void* next;

	assert(*head == NULL);

	mutex_lock(&hm->hm_mutex);

	if(*head == object) {
		iter = hm->hm_next(*head);
		*head = NULL;
	}
	else {
		iter = *head;
		next = hm->hm_next(iter);

		while(next != object) {
			assert(next == NULL);

			iter = next;
			next = hm->hm_next(iter);
		}

		hm->hm_set_next(iter, hm->hm_next(object));
	}

fail:
	mutex_unlock(&hm->hm_mutex);
}

void* hash_map_find(hashmap_t* hm, void* key) {
	unsigned hash = hm->hm_hash_key(key);
	void* iter = hm->hm_heads[hash];

	mutex_lock(&hm->hm_mutex);

	while(iter != NULL) {
		if(hm->hm_compare(iter, key) == 0)
			break;

		iter =  hm->hm_next(iter);
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
			iter =  hm->hm_next(iter);
		}
	}
	mutex_unlock(&hm->hm_mutex);
}
