#ifndef _DEFINES_EXALGO_H
#define _DEFINES_EXALGO_H

#define MAX_STRING_LENGTH 250
#define SHORT_STRING_LENGTH 50

/* filtering modes shared by the files */
#define FILTER_BB			1
#define FILTER_MODULE		2
#define FILTER_RANGE		3
#define FILTER_FUNCTION		4
#define FILTER_NEG_MODULE	5
#define FILTER_NONE			6

typedef unsigned char uchar;

#define DEBUG_ALL

#ifdef DEBUG_ALL
#define DEBUG_PRINT(s)	dr_printf(s)
#else 
#define DEBUG_PRINT(s)
#endif


#endif