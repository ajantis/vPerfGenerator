/*
 * tsinit.c
 *
 *  Created on: 30.12.2012
 *      Author: myaut
 */

#include <tsinit.h>

#include <stdio.h>
#include <stdlib.h>

int tsi_subsys_count;
struct subsystem* tsi_subsys;

void finish(void);

int init(struct subsystem* subsys, int count) {
	int i = 0;
	int err = 0;

	tsi_subsys = subsys;
	tsi_subsys_count = count;

	atexit(finish);

	if(plat_init() == -1) {
		fprintf(stderr, "Platform-depenent initialization failure!\n");
		exit(-1);
	}

	for(i = 0; i < count; ++i) {
		err = subsys[i].s_init();

		if(err != 0) {
			subsys[i].s_state = SS_ERROR;
			fprintf(stderr, "Failure initializing %s, exiting\n", subsys[i].s_name);
			exit(err);

			return err;
		}

		subsys[i].s_error_code = err;
	}

	return 0;
}

void finish(void) {
	int i = 0;

	for(i = tsi_subsys_count - 1; i >= 0; --i) {
		if(tsi_subsys[i].s_state == SS_OK) {
			tsi_subsys[i].s_fini();
		}
	}

	plat_finish();
}
