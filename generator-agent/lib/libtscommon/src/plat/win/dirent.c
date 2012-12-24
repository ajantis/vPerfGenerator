/*
 * dirent.c
 *
 *  Created on: 23.12.2012
 *      Author: myaut
 */

#include <defs.h>
#include <plat/win/dirent.h>

#include <mempool.h>

#include <assert.h>

PLATAPI plat_dir_t* plat_opendir(const char *name) {
	plat_dir_t* dirp = (plat_dir_t*) mp_malloc(sizeof(plat_dir_t));

	dirp->d_handle = NULL;

	strncpy(dirp->d_path, name, MAX_PATH);
	/* Should find all files in directory */
	strncpy(dirp->d_path, "*", MAX_PATH);

	return dirp;
}

PLATAPI plat_dir_entry_t* plat_readdir(plat_dir_t *dirp) {
	plat_dir_entry_t* d_entry = &dirp->d_entry;

	if(dirp->d_handle) {
		if(!FindNextFile(dirp->d_handle, &d_entry->d_find_data))
			return NULL;
	}
	else {
		dirp->d_handle = FindFirstFile(dirp->d_path,
				                       &d_entry->d_find_data);

		if(ERROR_INVALID_HANDLE == dirp->d_handle)
			return NULL;
	}

	/* Convert WIN32_FIND_DATA to dirent */
	strncpy(d_entry->d_name, d_entry->d_find_data.cFileName, 256);

	if(d_entry->d_find_data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) {
		d_entry->d_type = DT_CHR;
	}
	else if(d_entry->d_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		d_entry->d_type = DT_DIR;
	}
	else {
		d_entry->d_type = DT_REG;
	}

	return d_entry;
}

PLATAPI int plat_closedir(plat_dir_t *dirp) {
	FindClose(dirp->d_handle);

	mp_free(dirp);

	return 0;
}
