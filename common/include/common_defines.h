#ifndef _EXALGO_COMMON_DEFINES_H
#define _EXALGO_COMMON_DEFINES_H

#include <fstream>

#define MAX_STRING_LENGTH 500

extern bool debug;
extern uint32_t debug_level;
extern std::ofstream log_file;

#define ASSERT_MSG(x,s)	      \
	if(!(x)){				  \
		printf s;			  \
		exit(1);			  \
	}

#define DEBUG_PRINT(s,l)					\
	if(debug){								\
		if (l <= debug_level) { printf s; }	\
	}	


#define IF_PRINT(x,s) \
	  if((x)){        \
		printf s ;    \
	  } 

#define LOG_PRINT(file,l, ...)		     \
	if(debug_mode){						 \
		if( l <= debug_level){			 \
			fprintf(file, __VA_ARGS__);  \
		}								 \
	}									 


#endif