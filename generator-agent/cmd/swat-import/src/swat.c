/*
 * swat.c
 *
 *  Created on: Jan 17, 2013
 *      Author: myaut
 */

#include <swat.h>

#include <hashmap.h>
#include <pathutil.h>
#include <ilog2.h>

#include <stdio.h>
#include <stdlib.h>

#include <libjson.h>

ts_time_t quantum = 250 * T_MS;

char experiment_dir[PATHMAXLEN];

#ifdef SWAT_TRACE
boolean_t swat_wl_trace = B_FALSE;
boolean_t swat_wl_stat_trace = B_TRUE;

unsigned swat_chunk_count = 0;
unsigned swat_wl_count = 0;
long swat_wl_max_step = 0;

#define SWAT_WL_STAT_INC(s)	++s
#define SWAT_WL_STAT_MAX(s, v)  s = max(s, v)

#else

#define SWAT_WL_STAT_INC(s)
#define SWAT_WL_STAT_MAX(s, v)

#endif

DECLARE_HASH_MAP(swat_wl_hash_map, swat_workload_t, SWATWLHASHSIZE, swl_key, swl_next,
	/*hash*/ {
		swl_key_t* swl_key = (swl_key_t*) key;

		/* xfersize is more variable, so use it for hash */
		return ilog2l(swl_key->swl_xfersize) & SWATWLHASHMASK;
	},
	/*compare*/ {
		swl_key_t* swl_key1 = (swl_key_t*) key1;
		swl_key_t* swl_key2 = (swl_key_t*) key2;

		return (swl_key1->swl_device == swl_key2->swl_device) &&
				(swl_key1->swl_xfersize == swl_key2->swl_xfersize) &&
				(swl_key1->swl_flag == swl_key2->swl_flag);
	}
);

void swat_wl_add_chunk(swat_workload_t* wl) {
	swl_chunk_t* chunk = (swl_chunk_t*) malloc(sizeof(swl_chunk_t));

	list_add_tail(&chunk->swl_node, &wl->swl_num_rqs);

	wl->swl_last_chunk = chunk;

	SWAT_WL_STAT_INC(swat_chunk_count);

#ifdef SWAT_TRACE
	if(swat_wl_trace) {
		fprintf(stderr, "Added chunk to %s [step: %ld]\n", wl->swl_name, wl->swl_last_step);
	}
#endif
}

/**
 * Advances workload and return step number relative to last chunks
 * if steps is zero, simply calculates relative step number
 *
 * @param wl Workload where steps are adding
 * @param steps Number of steps*/
long swat_wl_add_steps(swat_workload_t* wl, long steps) {
	long rel_step = wl->swl_last_step % SWATCHUNKLEN;

#	ifdef SWAT_TRACE
	if(swat_wl_trace) {
		fprintf(stderr, "Adding %ld steps to %s [step: %ld]\n", steps, wl->swl_name, wl->swl_last_step);
	}
#	endif

	while(steps-- > 0) {
		wl->swl_last_step++;
		rel_step++;

		if(rel_step == SWATCHUNKLEN) {
			/* Should allocate new chunk */
			swat_wl_add_chunk(wl);
			rel_step = 0;
		}

		/* Initialize it to zero */
		wl->swl_last_chunk->swl_num_rqs[rel_step] = 0;
	}

	SWAT_WL_STAT_MAX(swat_wl_max_step, wl->swl_last_step);

	return rel_step;
}

/**
 * Create new workload*/
swat_workload_t* swat_wl_create(swl_key_t* key) {
	swat_workload_t* wl = (swat_workload_t*) malloc(sizeof(swat_workload_t));

	wl->swl_key.swl_device = key->swl_device;
	wl->swl_key.swl_xfersize = key->swl_xfersize;
	wl->swl_key.swl_flag = key->swl_flag;

	snprintf(wl->swl_name, SWATWLNAMELEN, "%s-%lld-%ld",
					(key->swl_flag == 1) ? "write" : "read",
					key->swl_xfersize, key->swl_device);

	wl->swl_last_step = 0;
	wl->swl_total_rqs = 0;

	wl->swl_next = 0;

	list_head_init(&wl->swl_num_rqs, "swl_num_rqs");

	/* Add first chunk to workload */
	swat_wl_add_chunk(wl);

	hash_map_insert(&swat_wl_hash_map, (hm_item_t*) wl);

	SWAT_WL_STAT_INC(swat_wl_count);

	return wl;
}

void swat_add_entry(struct swat_record* sr) {
	swat_workload_t* wl;
	swl_key_t key;

	long steps;
	long rel_step;

	key.swl_device = sr->sr_device;
	key.swl_xfersize = sr->sr_xfersize;
	key.swl_flag = sr->sr_flag;

	wl = hash_map_find(&swat_wl_hash_map, (hm_key_t*) &key);

	if(wl == NULL) {
		wl = swat_wl_create(&key);
	}

	/* Add entry to workload */
	steps = ((ts_time_t) (sr->sr_start * T_US) / quantum) - wl->swl_last_step;
	rel_step = swat_wl_add_steps(wl, steps);

	++wl->swl_last_chunk->swl_num_rqs[rel_step];
	++wl->swl_total_rqs;

#	ifdef SWAT_TRACE
	if(swat_wl_stat_trace) {
		fprintf(stderr, "wls: %4u chunks: %8u step: %ld \n",
					swat_chunk_count, swat_wl_count, swat_wl_max_step);
	}
#	endif
}

int swat_wl_init(void) {
	hash_map_init(&swat_wl_hash_map, "swat_wl_hash_map");

	return 0;
}

/**
 * Serialize swat data into JSON experiment data
 * ---------------------------------------------
 * */

JSONNODE* json_swat_wl_format(const char* dev_path, swat_workload_t* wl) {
	JSONNODE* node = json_new(JSON_NODE);
	JSONNODE* params = json_new(JSON_NODE);

	json_set_name(node, wl->swl_name);
	json_set_name(params, "params");

	json_push_back(node, json_new_a("module", "dummy"));
	json_push_back(node, json_new_a("threadpool", "[DEFAULT]"));
	json_push_back(node, params);

	json_push_back(params, json_new_i("blocksize", wl->swl_key.swl_xfersize));
	json_push_back(params, json_new_a("test",
			(wl->swl_key.swl_flag == 1) ? "write" : "read"));
	json_push_back(params, json_new_i("path", dev_path));
	json_push_back(params, json_new_b("sparse", B_FALSE));

	return node;
}

void swat_wl_dump_steps(FILE* steps, swat_workload_t* wl) {
	long rel_step = 0;
	long step = 0;

	swl_chunk_t* chunk;

	list_for_each_entry(swl_chunk_t, chunk, &wl->swl_num_rqs, swl_node) {
		for(rel_step = 0; rel_step < SWATCHUNKLEN; ++rel_step) {
			if(step > wl->swl_last_step) {
				break;
			}

			fprintf(steps, "%u\n", chunk->swl_num_rqs[rel_step]);

			++step;
		}
	}
}

int swat_wl_write_steps(const char* steps_filename, swat_workload_t* wl) {
	char steps_path[PATHMAXLEN];

	FILE* steps;

	path_join(steps_path, PATHMAXLEN, experiment_dir, steps_filename, NULL);

	steps = fopen(steps_path, "w");

	if(!steps) {
		fprintf(stderr, "Failed to open file %s\n", steps_path);
		return -1;
	}

	swat_wl_dump_steps(steps, wl);

	fclose(steps);

	return 0;
}

/* Passed into walker */
struct swat_wl_ser_context {
	JSONNODE* steps;
	JSONNODE* workloads;

	boolean_t error;
};

int swat_wl_ser_walker(hm_item_t* obj, void* arg) {
	swat_workload_t* wl = (swat_workload_t*) obj;
	struct swat_wl_ser_context* context = (struct swat_wl_ser_context*) arg;

	char steps_filename[PATHMAXLEN];

	char* dev_path = swat_get_mapping(wl->swl_key.swl_device);

	/* Ignore devices that do not have mappings */
	if(dev_path) {
		snprintf(steps_filename, PATHMAXLEN, "steps-%s.tss", wl->swl_name);

		/* Serialize steps */
		if(swat_wl_write_steps(steps_filename, wl) != 0) {
			context->error = B_TRUE;
			return HM_WALKER_STOP;
		}

		json_push_back(context->steps, json_new_a(wl->swl_name, steps_filename));
		json_push_back(context->workloads, json_swat_wl_format(dev_path, wl));
	}

	printf("Workload %s [%lu requests in %ld steps] is %s\n", wl->swl_name,
			wl->swl_total_rqs, wl->swl_last_step,
			dev_path ? "serialized" : "ignored");

	free(obj);

	return HM_WALKER_CONTINUE | HM_WALKER_REMOVE;
}

/**
 * Serialize experiment into file
 * */
int swat_wl_serialize(void) {
	JSONNODE* node = json_new(JSON_NODE);
	struct swat_wl_ser_context context;

	char experiment_path[PATHMAXLEN];
	FILE* experiment;
	char* exp_str;

	path_join(experiment_path, PATHMAXLEN, experiment_dir, "experiment.json", NULL);

	experiment = fopen(experiment_path, "w");

	if(!experiment) {
		fprintf(stderr, "Failed to open file %s\n", experiment_path);
		return SWAT_ERR_SERIALIZE;
	}

	context.steps = json_new(JSON_NODE);
	context.workloads = json_new(JSON_NODE);
	context.error = B_FALSE;

	json_set_name(context.steps, "steps");
	json_set_name(context.workloads, "workloads");

	hash_map_walk(&swat_wl_hash_map, swat_wl_ser_walker, &context);

	if(context.error) {
		fclose(experiment);
		return SWAT_ERR_SERIALIZE;
	}

	/* Write experiment */
	json_push_back(node, json_new_a("name", "swat_experiment"));
	json_push_back(node, context.steps);
	json_push_back(node, context.workloads);

	exp_str = json_write_formatted(node);
	fputs(exp_str, experiment);
	json_free(exp_str);

	return 0;
}
