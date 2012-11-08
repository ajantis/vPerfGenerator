/*
 * wlparam.h
 *
 *  Created on: 08.11.2012
 *      Author: myaut
 */

#ifndef WLPARAM_H_
#define WLPARAM_H_

#include <stddef.h>
#include <defs.h>

/* Workload parameter declaration
 *
 * These declarations used to provide module description to server
 * and simply parse workload parameters in JSON format.
 *
 * Each parameter declaration has:
 * 	- type @see wlp_type_t
 * 	- limitations (like string max length, number range, possible values, etc.) @see wlp_range_t and macroses
 * 	- name and description (text)
 * 	- offset in control structure which is used during parsing
 *
 * Parameter declaration are saved as array of wlp_descr_t structures with "NULL-terminator" with type WLP_NULL.
 * */

typedef enum {
	/*NULL-term*/	WLP_NULL,		/*Used to mark end of param list*/

	/*long*/		WLP_INTEGER,
	/*double*/		WLP_FLOAT,
	/*char[]*/ 		WLP_RAW_STRING, /*Any string*/

	/*size_t*/		WLP_SIZE,		/*size_t. May be 4k, 4M or 4G*/
	/*char[]*/		WLP_STRING_SET  /*string - one of posible value*/
} wlp_type_t;

typedef struct {
	int range;		/*enabled flag*/

	union {
		/*WLP_RAW_STRING*/
		struct {
			unsigned str_length;
		};
		/*WLP_INTEGER*/
		struct {
			long i_min;
			long i_max;
		};
		/*WLP_FLOAT*/
		struct {
			double d_min;
			double d_max;
		};
		/*WLP_SIZE*/
		struct {
			size_t sz_min;
			size_t sz_max;
		};
		/*WLP_STRING_SET*/
		struct {
			int ss_num;
			char** ss_strings;
		};
	};
} wlp_range_t;

#define WLP_NO_RANGE()	{ .range = FALSE }

#define WLP_STRING_LENGTH(length) { .range = TRUE, .str_length = length}

#define WLP_INT_RANGE(min, max)  { .range = TRUE, .i_min = min, .i_max = max }
#define WLP_FLOAT_RANGE(min, max)  { .range = TRUE, .d_min = min, .d_max = max }
#define WLP_SIZE_RANGE(min, max)  { .range = TRUE, .sz_min = min, .sz_max = max }

/*Description of param*/
typedef struct {
	wlp_type_t type;
	wlp_range_t range;

	char* name;
	char* description;

	/*Offset in data structure where param value would be written*/
	size_t off;
} wlp_descr_t;

#endif /* WLPARAM_H_ */
