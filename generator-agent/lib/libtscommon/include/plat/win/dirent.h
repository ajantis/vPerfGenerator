/*
 * dirent.h
 *
 *  Created on: 23.12.2012
 *      Author: myaut
 */

#ifndef PLAT_WIN_DIRENT_H_
#define PLAT_WIN_DIRENT_H_

#include <windows.h>

#define DT_UNKNOWN      0
#define DT_FIFO         1
#define DT_CHR          2
#define DT_DIR          4
#define DT_BLK          6
#define DT_REG          8
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14

typedef struct {
	WIN32_FIND_DATA d_find_data;

	unsigned char  d_type;      /* type of file; not supported
								  by all file system types */
	char           d_name[256]; /* filename */
} plat_dir_entry_t;

typedef struct {
	HANDLE 	d_handle;

	char 	d_path[MAX_PATH];

	plat_dir_entry_t d_entry;	/* Current Entry */
} plat_dir_t;

#endif /* PLAT_WIN_DIRENT_H_ */
