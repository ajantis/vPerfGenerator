/*
 * tsdirent.h
 *
 *  Created on: 23.12.2012
 *      Author: myaut
 */

#ifndef TSDIRENT_H_
#define TSDIRENT_H_

#include <defs.h>
#include <plat/dirent.h>

LIBEXPORT PLATAPI plat_dir_t* plat_opendir(const char *name);
LIBEXPORT PLATAPI plat_dir_entry_t* plat_readdir(plat_dir_t *dirp);
LIBEXPORT PLATAPI int plat_closedir(plat_dir_t *dirp);

#endif /* DIRENT_H_ */
