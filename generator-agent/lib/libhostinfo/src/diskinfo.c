/*
 * diskinfo.c
 *
 *  Created on: 27.04.2013
 *      Author: myaut
 */

#include <defs.h>
#include <mempool.h>
#include <diskinfo.h>

#include <string.h>

/**
 * diskinfo.c - reads information about disks from operating system
 *
 * Disks form hierarchy:
 * 		- top level - volumes provided by software volume managers (HI_DSKT_VOLUME)
 * 		- 2nd level - volume manager pools (HI_DSKT_POOL)
 * 		- 3rd level - disks (HI_DSKT_DISK)
 * 		- bottom level - partions (HI_DSKT_PARTITION)
 * However pools may be built from partitions, and so on.
 *
 * SLAVES
 *
 * Child disks are called slaves (came from linux /sys/block terminology),
 * each slave is handled by struct hi_dsk_slave. Some slaves has only one parent
 * (i.e. partitions), so they use hi_dsk_slave that embedded into hi_dsk_info_t
 * structure: di->d_slave_node. Other (ususally pools) - have many parents,
 * so in this case hi_dsk_slave allocated from heap.
 *
 * I.e. for my desktop:
 *
 *                    +----------------------+
 *                    |                      |
 * /dev/sda           |  /dev/sda1           |  /dev/sda2
 * hi_dsk_info_t <----+  hi_dsk_info_t <---+ |  hi_dsk_info_t <----+
 * +---------------+  |  +---------------+ | |  +---------------+  |
 * |  DISK         |  |  |  PARTITION    | | |  |  PARTITION    |  |
 * |               |  |  +---------------+ | |  +---------------+  |
 * |               |  |  | d_slave_node  | | |  | d_slave_node  |  |
 * | d_slave_list <------->.ds_node    <---------> .ds_node  <---------- ...
 * |               |  +----.ds_parent    | | +---- .ds_parent   |  |
 * |               |     | .ds_slave ------+    |  .ds_slave ------+
 * |               |     |               |      |               |
 * +---------------+     +---------------+      +---------------+
 *                       |               |      |               |
 *                       +---------------+      +---------------+
 *
 * and:
 *
 * /dev/dm-1             /dev/dm-0
 * hi_dsk_info_t <----+  hi_dsk_info_t <---+-+
 * +---------------+  |  +---------------+ | |
 * |  VOLUME       |  |  |  POOL         | | |
 * |               |  |  +---------------+ | |
 * |               |  |  | d_slave_node  | | |
 * | d_slave_list <------->.ds_node      | | |
 * |               |  +----.ds_parent    | | |
 * |               |     | .ds_slave ------+ |
 * |               |     |               |   |
 * +---------------+     +---------------+   |
 *                       |               |   |
 *                       +---------------+   |
  * /dev/dm-1                                |
 * hi_dsk_info_t <----+                      |
 * +---------------+  |                      |
 * |  VOLUME       |  |   hi_dsk_info_t      |
 * |               |  |  +---------------+   |
 * |               |  |  | d_slave_node  |   |
 * | d_slave_list <------->.ds_node      |   |
 * |               |  +----.ds_parent    |   |
 * |               |     | .ds_slave --------+
 * |               |     |               |
 * +---------------+     +---------------+
 *
 * */

int hi_dsk_probe_state = HI_DSK_NOT_PROBED;
list_head_t hi_dsk_head;

mp_cache_t hi_dsk_cache;

#ifdef HOSTINFO_TRACE
boolean_t hi_dsk_trace = B_TRUE;
#endif

/**
 * Initialize slave node
 * */
void hi_dsk_slave_init(struct hi_dsk_slave* ds) {
	ds->ds_type = HI_DS_ROOT;

	ds->ds_parent = NULL;
	ds->ds_di = NULL;

	list_node_init(&ds->ds_node);
}

/**
 * Allocate and initialize disk descriptor hi_dsk_info_t
 * */
hi_dsk_info_t* hi_dsk_create(void) {
	hi_dsk_info_t* di = mp_cache_alloc(&hi_dsk_cache);

	di->d_path[0] = '\0';
	di->d_size = 0;
	di->d_mode = 0;
	di->d_type = HI_DSKT_UNKNOWN;

	di->d_port[0] = '\0';
	di->d_model[0] = '\0';
	di->d_fs[0] = '\0';

	list_head_init(&di->d_slave_list, "hi_dsk_slaves");
	list_node_init(&di->d_node);

	hi_dsk_slave_init(&di->d_slave_node);

	return di;
}

/**
 * Destroy disk descriptor and free slave handles if needed
 * */
void hi_dsk_destroy(hi_dsk_info_t* di) {
	struct hi_dsk_slave *ds, *temp;

	/* Free slaves list if slave handlers was allocated */
	disk_for_each_slave_safe(di, ds, temp) {
		if(ds->ds_type == HI_DS_ALLOCATED)
			mp_free(ds);
	}

	mp_cache_free(&hi_dsk_cache, di);
}

/**
 * Destroy all disk descriptors and reinitialize
 * global descriptor list
 * */
void hi_dsk_destroy_all(void) {
	hi_dsk_info_t *di, *temp;
	list_head_t* head = &hi_dsk_head;

	list_for_each_entry_safe(hi_dsk_info_t, di, temp, head, d_node) {
		hi_dsk_destroy(di);
	}

	list_head_init(&hi_dsk_head, "hi_dsk_list");
}

/**
 * Find disk by it's name
 * @param name - name of disk
 *
 * @return disk descriptor or NULL if it wasn't found
 * */
hi_dsk_info_t* hi_dsk_find(const char* name) {
	hi_dsk_info_t* di;
	list_head_t* head =  &hi_dsk_head;

	list_for_each_entry(hi_dsk_info_t, di, head, d_node) {
		if(strcmp(di->d_name, name) == 0)
			return di;
	}

	return NULL;
}

/**
 * Add disk descriptor to global list
 */
void hi_dsk_add(hi_dsk_info_t* di) {
	hi_dsk_dprintf("hi_dsk_add: name: %s path: %s\n", di->d_name, di->d_path);

	list_add_tail(&di->d_node, &hi_dsk_head);
}

/**
 * Attach disk descriptor to parent as a slave
 *
 * @param di		Slave disk descriptor
 * @param parent    Parent disk descriptor
 */
void hi_dsk_attach(hi_dsk_info_t* di, hi_dsk_info_t* parent) {
	struct hi_dsk_slave* ds;

	hi_dsk_dprintf("hi_dsk_attach: di: %s parent: %s\n", di->d_name, parent->d_name);

	if(di->d_slave_node.ds_di == NULL) {
		/* This is first parent for this di,
		 * so simply use slave_node from it */
		ds = &di->d_slave_node;
		ds->ds_type = HI_DS_EMBEDDED;
	}
	else {
		/* Allocate new slave node */
		ds = mp_malloc(sizeof(struct hi_dsk_slave));
		ds->ds_type = HI_DS_ALLOCATED;
	}

	ds->ds_di = di;
	ds->ds_parent = parent;

	list_add_tail(&ds->ds_node, &parent->d_slave_list);
}

/**
 * Probe system's disk (if needed) and return pointer
 * to global disk descriptor list head
 *
 * @param reprobe Probe system's disks again
 *
 * @return pointer to head or NULL if probe failed
 */
list_head_t* hi_dsk_list(boolean_t reprobe) {
	if(hi_dsk_probe_state == HI_DSK_NOT_PROBED || reprobe) {
		if(reprobe) {
			hi_dsk_destroy_all();
		}

		hi_dsk_dprintf("hi_dsk_list: probing disks\n");

		hi_dsk_probe_state = hi_dsk_probe();
	}

	if(hi_dsk_probe_state == HI_DSK_PROBE_ERROR)
		return NULL;

	return &hi_dsk_head;
}

int hi_dsk_init(void) {
	list_head_init(&hi_dsk_head, "hi_dsk_list");
	mp_cache_init(&hi_dsk_cache, hi_dsk_info_t);

	return 0;
}

void hi_dsk_fini(void) {
	hi_dsk_destroy_all();
	mp_cache_destroy(&hi_dsk_cache);
}
