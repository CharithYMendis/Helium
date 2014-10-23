#ifndef _DEFINES_EXALGO_H
#define _DEFINES_EXALGO_H

#define MAX_STRING_LENGTH 500
#define SHORT_STRING_LENGTH 50

/* filtering modes shared by the files */
#define FILTER_BB			1
#define FILTER_MODULE		2
#define FILTER_RANGE		3
#define FILTER_FUNCTION		4
#define FILTER_NEG_MODULE	5
#define FILTER_NONE			6
#define FILTER_NUDGE		7

typedef unsigned char uchar;

#define DEBUG_PRINT(...)		 \
	if(debug_mode){				 \
		dr_printf(__VA_ARGS__);  \
	}							 \

#define LOG_PRINT(file, ...)		     \
	if(log_mode){						 \
		dr_fprintf(file,__VA_ARGS__);    \
	}									 \


#endif