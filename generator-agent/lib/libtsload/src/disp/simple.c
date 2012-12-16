/*
 * simple.c
 *
 *  Created on: 04.12.2012
 *      Author: myaut
 */

#include <disp.h>

disptime_t simple_disp_interval(void) {
	return 0;
}

disp_t simple_disp = {
	.disp_name = "simple",
	.disp_interval = simple_disp_interval
};
