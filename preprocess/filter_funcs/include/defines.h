#ifndef _DEFINES_H
#define _DEFINES_H

#define MAX_STRING_LENGTH  500

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(s) printf(s)
#else
#define DEBUG_PRINT(s)
#endif

#endif