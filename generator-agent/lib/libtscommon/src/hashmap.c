/*
 * hashmap.c
 *
 *  Created on: 25.11.2012
 *      Author: myaut
 */

#include <hashmap.h>
#include <threads.h>

#include <assert.h>

/* Helper routines for hash map */
STATIC_INLINE hm_item_t* hm_next(hashmap_t* hm, hm_item_t* obj) {
	return *((hm_item_t**) (obj + hm->hm_off_next));
}

STATIC_INLINE void hm_set_next(hashmap_t* hm, hm_item_t* obj, hm_item_t* next) {
	*((hm_item_t**) (obj + hm->hm_off_next)) = next;
}

STATIC_INLINE hm_key_t* hm_get_key(hashmap_t* hm, hm_item_t* obj) {
	return (obj + hm->hm_off_key);
}

STATIC_INLINE unsigned hm_hash_object(hashmap_t* hm, hm_item_t* obj) {
	return hm->hm_hash_key(hm_get_key(hm, obj));
}

/**
 * Initialize hash map.
 *
 * Because hash map usually a global object, it doesn't
 * provide formatted names
 *
 * @param hm hash map to initialize
 * @param name identifier for hash map
 * */
void hash_map_init(hashmap_t* hm, const char* name) {
	int i = 0;

	for(i = 0; i < hm->hm_size; ++i) {
		hm->hm_heads[i] = NULL;
	}

	mutex_init(&hm->hm_mutex, "hm-%s", name);
}

/**
 * Destroy hash map
 *
 * It shouldn't contain objects (or assertion will rise)
 */
void hash_map_destroy(hashmap_t* hm) {
	int i = 0;

	for(i = 0; i < hm->hm_size; ++i) {
		assert(hm->hm_heads[i] == NULL);
	}

	mutex_destroy(&hm->hm_mutex);
}

/**
 * Insert element into hash map
 *
 * @param hm hash map
 * @param object object to be inserted
 *
 * @return HASH_MAP_OK if object was successfully inserted or HASH_MAP_DUPLICATE if \
 * object with same key (not hash!) already exists in hash map
 * */
int hash_map_insert(hashmap_t* hm, hm_item_t* object) {
	unsigned hash = hm_hash_object(hm, object);
	int ret = HASH_MAP_OK;

	hm_item_t** head = hm->hm_heads + hash;
	hm_item_t* iter;
	hm_item_t* next;

	mutex_lock(&hm->hm_mutex);
	if(*head == NULL) {
		*head = object;
	}
	else {
		iter = *head;
		next = hm_next(hm, iter);

		/* Walk through chain until reach tail */
		while(B_TRUE) {
			if(hm->hm_compare(hm_get_key(hm, iter),
			                  hm_get_key(hm, object))) {
				ret = HASH_MAP_DUPLICATE;
				goto done;
			}

			/* Found tail */
			if(next == NULL)
				break;

			iter = next;
			next = hm_next(hm, iter);
		}

		/* Insert object into tail */
		hm_set_next(hm, iter, object);
	}

done:
	mutex_unlock(&hm->hm_mutex);

	return ret;
}

/**
 * Remove element from hash map
 *
 * @param hm hash map
 * @param object object to be removed
 *
 * @return HASH_MAP_OK if object was successfully remove or HASH_MAP_NOT_FOUND if \
 * object is not
 * */
int hash_map_remove(hashmap_t* hm, hm_item_t* object) {
	unsigned hash = hm_hash_object(hm, object);
	int ret = HASH_MAP_OK;

	hm_item_t** head = hm->hm_heads + hash;
	hm_item_t* iter;
	hm_item_t* next;

	mutex_lock(&hm->hm_mutex);

	assert(*head != NULL);

	if(*head == object) {
		iter = hm_next(hm, *head);
		*head = iter;
	}
	else {
		iter = *head;
		next = hm_next(hm, iter);

		/* Find object that is previous for
		 * object we are removing and save it to iter */
		while(next != object) {
			if(next == NULL) {
				ret = HASH_MAP_NOT_FOUND;
				goto done;
			}

			iter = next;
			next = hm_next(hm, iter);
		}

		/* Remove object from chain */
		hm_set_next(hm, iter, hm_next(hm, object));
	}

done:
	mutex_unlock(&hm->hm_mutex);
	return ret;
}

/**
 * Find object in hash map by key
 */
void* hash_map_find(hashmap_t* hm, const hm_key_t* key) {
	unsigned hash = hm->hm_hash_key(key);
	hm_item_t* iter = hm->hm_heads[hash];

	mutex_lock(&hm->hm_mutex);

	while(iter != NULL) {
		if(hm->hm_compare(hm_get_key(hm, iter), key))
			break;

		iter =  hm_next(hm, iter);
	}

	mutex_unlock(&hm->hm_mutex);

	return iter;
}

/**
 * Walk over objects in hash map and execute func for each object.
 *
 * Func proto: int (*func)(void* object, void* arg)
 * Function func may return HM_WALKER_CONTINUE or HM_WALKER_STOP.
 *
 * For HM_WALKER_STOP, hash_map_walk will stop walking over hash map and return
 * current object
 *
 * @note because of nature of hash maps, walking order is undefined
 *
 * @param hm hash map
 * @param func function that will be called for each object
 * @param arg argument that will be passed as second argument for func
 *
 * @return NULL or object where func returned STOP
 */
void* hash_map_walk(hashmap_t* hm, int (*func)(hm_item_t* object, void* arg), void* arg) {
	int i = 0;
	int ret = 0;
	hm_item_t* iter = NULL;
	hm_item_t* next = NULL;
	hm_item_t* prev = NULL;

	mutex_lock(&hm->hm_mutex);
	for(i = 0; i < hm->hm_size; ++i) {
		prev = NULL;
		next = NULL;
		iter = hm->hm_heads[i];

		while(iter != NULL) {
			next = hm_next(hm, iter);

			ret = func(iter, arg);

			if(ret & HM_WALKER_REMOVE) {
				if(prev == NULL) {
					/* Entire chain before iter was removed, so this is head */
					hm->hm_heads[i] = next;
				}
				else {
					hm_set_next(hm, prev, next);
				}
			}
			else {
				/* Save prev pointer */
				prev = iter;
			}

			if(ret & HM_WALKER_STOP)
				goto done;

			iter = next;
		}

		if(prev == NULL) {
			/* Entire chain was removed */
			hm->hm_heads[i] = NULL;
		}
	}

	iter = NULL;

done:
	mutex_unlock(&hm->hm_mutex);

	return iter;
}
