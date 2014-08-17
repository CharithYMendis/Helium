#ifndef _EXALGO_IMAGEINFO_H
#define _EXALGO_IMAGEINFO_H

#include <Windows.h>
#include "defines.h"
#include <stdint.h>
#include <string>
#include <gdiplus.h>


typedef unsigned char byte;

using namespace std;

struct image_t{

	uint32_t height;
	uint32_t width;
	uint32_t colors; /* number of colors in the image */
	uint32_t bits_per_color;
	uint32_t bits_per_pixel;
	uint32_t is_alpha;
	byte * image_array; /* this is just a stream of bytes - no type information column major format */


};

ULONG_PTR initialize_image_subsystem();
void shutdown_image_subsystem(ULONG_PTR token);
image_t * populate_imageinfo(const char * filename);

byte * get_image_buffer(Gdiplus::Bitmap * image, uint32_t * height, uint32_t * width, uint32_t * fields);
void update_image_buffer(Gdiplus::Bitmap * image, byte * array);
void save_image(Gdiplus::Bitmap * image, wchar_t * file);



#endif