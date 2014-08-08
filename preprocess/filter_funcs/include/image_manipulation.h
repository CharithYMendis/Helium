#ifndef _IMAGE_MANI_H
#define _IMAGE_MANI_H

#include <windows.h>
#include <string>
#include <gdiplus.h>
#include <iostream>
#include <stdint.h>
#include <wchar.h>
#include <string>
#include <stdio.h>
#include <algorithm>
#include <string.h>


struct image_t {
	uint32_t width;
	uint32_t height;
};

ULONG_PTR initialize_image_subsystem();
void shutdown_image_subsystem(ULONG_PTR token);
image_t * populate_imageinfo(const char * filename);

#endif