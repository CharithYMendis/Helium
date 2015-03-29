#ifndef _EXALGO_COMMON_DEFINES_H
#define _EXALGO_COMMON_DEFINES_H

#include <fstream>
#include <stdint.h>

#define MAX_STRING_LENGTH 2000

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
	if(debug){							 \
		if( l <= debug_level){			 \
			fprintf(file, __VA_ARGS__);  \
		}								 \
	}		

#define LOG(file,...)						\
	if(debug){								\
		file << __VA_ARGS__ ;				\
	}


#endif