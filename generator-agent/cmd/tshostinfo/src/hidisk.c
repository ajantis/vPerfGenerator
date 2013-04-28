/*
 * hidisk.c
 *
 *  Created on: 27.04.2013
 *      Author: myaut
 */

#include <plat/posixdecl.h>
#include <diskinfo.h>
#include <hiprint.h>

#include <stdio.h>

char* disk_get_type(hi_dsk_info_t* di) {
	switch(di->d_type) {
	case HI_DSKT_UNKNOWN:
		return "???";
	case HI_DSKT_DISK:
		return "[disk]";
	case HI_DSKT_POOL:
		return "[pool]";
	case HI_DSKT_PARTITION:
		return "[part]";
	case HI_DSKT_VOLUME:
		return "[vol]";
	}

	return "???";
}

#define PRINT_DISK_XSTR(what, str) 					\
			if(str[0] != '\0')						\
				printf("\t%-7s: %s\n", what, str);

void print_disk_slaves(hi_dsk_info_t* di) {
	struct hi_dsk_slave* ds;
	char slaves[256] = "";

	char *ps = slaves;
	int off = 0;

	if(!list_empty(&di->d_slave_list)) {
		disk_for_each_slave(di, ds) {
			if((off + strlen(ds->ds_di->d_name) > 254))
				break;

			off += sprintf(slaves + off, " %s", ds->ds_di->d_name);
		}

		PRINT_DISK_XSTR("slaves", slaves);
	}
}

int print_disk_info(int flags) {
	list_head_t* disk_list;
	hi_dsk_info_t* di;

	disk_list = hi_dsk_list(B_FALSE);

	if(disk_list == NULL)
		return INFO_ERROR;

	print_header(flags, "DISK INFORMATION");

	if(flags & INFO_LEGEND) {
		printf("%-12s %-6s %-16s %c%c %s\n", "NAME", "TYPE",
				"SIZE", 'R', 'W', "PATH");
	}

	list_for_each_entry(hi_dsk_info_t, di, disk_list, d_node) {
		printf("%-12s %-6s %-16llu %c%c %s\n", di->d_name,
					disk_get_type(di), di->d_size,
					(di->d_mode & R_OK)? 'R' : '-',
					(di->d_mode & W_OK)? 'W' : '-',
					di->d_path);

		if(!(flags & INFO_XDATA))
			continue;

		if(di->d_type != HI_DSKT_PARTITION) {
			PRINT_DISK_XSTR("id", di->d_id);
			PRINT_DISK_XSTR("bus", di->d_bus_type);
			PRINT_DISK_XSTR("port", di->d_port);

			print_disk_slaves(di);
		}

		if(di->d_type == HI_DSKT_DISK)
			PRINT_DISK_XSTR("model", di->d_model);
	}

	return INFO_OK;
}
