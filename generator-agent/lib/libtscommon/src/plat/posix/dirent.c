/*
 * dirent.c
 *
 *  Created on: 23.12.2012
 *      Author: myaut
 */

#include <defs.h>
#include <plat/dirent.h>

PLATAPI plat_dir_t* plat_opendir(const char *name) {
	return opendir(name);
}

PLATAPI plat_dir_entry_t* plat_readdir(plat_dir_t *dirp) {
	return readdir(dirp);
}

PLATAPI int plat_closedir(DIR *dirp) {
	return closedir(dirp);
}
