#ifndef _IMAGE_MANI_H
#define _IMAGE_MANI_H


#include <windows.h>
#include <string>
#include <gdiplus.h>
#include <iostream>
#include <stdint.h>
#include <wchar.h>

using namespace std;
using namespace Gdiplus;

ULONG_PTR initialize_image_subsystem();
void shutdown_image_subsystem(ULONG_PTR token);

byte * get_image_buffer(Bitmap * image, uint32_t * height, uint32_t * width, uint32_t * fields);
void update_image_buffer(Bitmap * image, byte * array);
void save_image(Bitmap *image, wchar_t * file);


#define COLORS	3


#endif