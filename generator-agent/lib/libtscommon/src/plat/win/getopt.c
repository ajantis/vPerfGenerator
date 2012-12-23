/*
 * getopt.c
 *
 *  Created on: 22.12.2012
 *      Author: myaut
 */

#include <defs.h>
#include <stdlib.h>

#include <getopt.h>

int opterr = 0;
int optind = 0;
int optopt = 0;
char *optarg = NULL;

PLATAPI int plat_getopt(int argc, const char* argv[], const char* options) {

}
