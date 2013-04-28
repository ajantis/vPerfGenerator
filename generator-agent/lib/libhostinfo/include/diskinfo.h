/*
 * diskinfo.h
 *
 *  Created on: 27.04.2013
 *      Author: myaut
 */

#ifndef DISKINFO_H_
#define DISKINFO_H_

#include <defs.h>
#include <list.h>

#define		HI_DSK_NOT_PROBED	0
#define		HI_DSK_PROBE_OK		1
#define		HI_DSK_PROBE_ERROR	-1

typedef enum hi_dsk_type {
	HI_DSKT_UNKNOWN,

	HI_DSKT_DISK,
	HI_DSKT_PARTITION,
	HI_DSKT_POOL,
	HI_DSKT_VOLUME
} hi_dsk_type_t;

struct hi_dsk_info;

enum hi_dsk_slave_type {
	HI_DS_ROOT,
	HI_DS_EMBEDDED,
	HI_DS_ALLOCATED
};

struct hi_dsk_slave {
	enum hi_dsk_slave_type ds_type;

	struct hi_dsk_info* ds_parent;
	struct hi_dsk_info* ds_di;

	list_node_t ds_node;
};

#define HIDSKNAMELEN	32
#define HIDSKPATHLEN	256
#define HIDSKBUSLEN		16
#define HIDSKPORTLEN	256
#define HIDSKMODELLEN	64
#define HIDSKFSLEN		256
#define HIDSKIDLEN		128

typedef struct hi_dsk_info {
	/* Mandatory fields */
	char d_name[HIDSKNAMELEN];
	char d_path[HIDSKPATHLEN];
	int d_mode;
	uint64_t d_size;
	hi_dsk_type_t d_type;

	/* Optional fields */
	char d_bus_type[HIDSKBUSLEN];

	/* For iSCSI LUNs - IQN,
	   For FC LUNs - WWN
	   For SCSI disks - bus/target/LUN
	   For Vol managers - internal ID */
	char d_port[HIDSKPORTLEN];

	char d_id[HIDSKIDLEN];
	char d_model[HIDSKMODELLEN];
	char d_fs[HIDSKFSLEN];	/* FS mounted here */

	/* Internal fields */
	list_head_t	d_slave_list;
	struct hi_dsk_slave d_slave_node;
	list_node_t d_node;
} hi_dsk_info_t;

PLATAPI int hi_dsk_probe(void);

hi_dsk_info_t* hi_dsk_create(void);
hi_dsk_info_t* hi_dsk_find(const char* name);

void hi_dsk_add(hi_dsk_info_t* di);
void hi_dsk_attach(hi_dsk_info_t* di, hi_dsk_info_t* parent);

LIBEXPORT list_head_t* hi_dsk_list(boolean_t reprobe);

LIBEXPORT int hi_dsk_init(void);
LIBEXPORT void hi_dsk_fini();

#define disk_for_each_slave(di, ds) 			\
	list_for_each_entry(struct hi_dsk_slave, ds, &di->d_slave_list, ds_node)
#define disk_for_each_slave_safe(di, ds, temp)	\
	list_for_each_entry_safe(struct hi_dsk_slave, ds, temp, &di->d_slave_list, ds_node)

#ifdef HOSTINFO_TRACE
extern boolean_t hi_dsk_trace;

#include <stdio.h>
#define hi_dsk_dprintf( ... )	\
			if(hi_dsk_trace) printf(  __VA_ARGS__ )
#else
#define hi_dsk_dprintf( ... )
#endif


#endif /* DISKINFO_H_ */
