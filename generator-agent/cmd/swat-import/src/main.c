/*
 * main.c
 *
 *  Created on: Jan 17, 2013
 *      Author: myaut
 */

#include <defs.h>
#include <getopt.h>
#include <pathutil.h>

#include <swatfile.h>
#include <swat.h>
#include <swatmap.h>

#include <zlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

extern char flat_filename[];
extern char experiment_dir[];

extern boolean_t conv_endianess;

void usage() {
	fprintf(stderr, "command line: \n"
					"\trun-tsload [-c] -f <flatfile.bin.gz> -e <experiment directory> -m <dev=path>\n"
					"\t\t process flatfile\n"
					"\t\t -c - convert endianess\n"
					"\trun-tsload -h \n"
					"\t\thelp\n");
}

void parse_options(int argc, const char* argv[]) {
	boolean_t eflag = B_FALSE;
	boolean_t fflag = B_FALSE;
	boolean_t ok = B_TRUE;

	time_t now;

	char* eq_pos;
	uint64_t dev;

	int c;

	char logname[48];

	while((c = plat_getopt(argc, argv, "e:f:m:ch")) != -1) {
		switch(c) {
		case 'h':
			usage();
			exit(0);
			break;
		case 'c':
			conv_endianess = B_TRUE;
			break;
		case 'e':
			eflag = B_TRUE;
			strncpy(experiment_dir, optarg, PATHMAXLEN);
			break;
		case 'm':
			eq_pos = strchr(optarg, '=');

			if(eq_pos == NULL) {
				fprintf(stderr, "-m argument should contain equal sign.\n", optopt);
				ok = B_FALSE;
				break;
			}

			*eq_pos++ = '\0';
			dev = atoll(optarg);
			swat_add_mapping(dev, eq_pos);

			break;
		case 'f':
			fflag = B_TRUE;
			strncpy(flat_filename, optarg, PATHMAXLEN);
			break;
		case '?':
			if(optopt == 'e' || optopt == 'f')
				fprintf(stderr, "-%c option requires an argument\n", optopt);
			else
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			ok = B_FALSE;
			break;
		}

		if(!ok)
			break;
	}

	if(!ok || !eflag || !fflag) {
		if(!eflag || !fflag)
			fprintf(stderr, "-e and -f flags should be used together\n");
		usage();
		exit(1);
	}
}

int main(int argc, const char* argv[]) {
	int ret = 0;

	swat_wl_init();
	swat_map_init();

	parse_options(argc, argv);

	if(swat_read() != SWAT_OK) {
		ret = 1;
		goto exit;
	}

	if(swat_wl_serialize() != SWAT_OK) {
		ret = 1;
		goto exit;
	}

exit:
	swat_map_fini();

	return ret;
}
