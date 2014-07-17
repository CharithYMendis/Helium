#ifndef _IMAGE_PARSER_H
#define _IMAGE_PARSER_H

#include "defines.h"
#include "canonicalize.h"
#include <stdint.h>
#include <iostream>
#include <fstream>

typedef unsigned char byte;

using namespace std;

struct Image{

	uint height;
	uint width;
	uint colors; /* number of colors in the image */
	uint bits_per_color;
	uint bits_per_pixel;
	uint is_alpha;
	byte * image_array; /* this is just a stream of bytes - no type information */


};

Image * parse_image_from_file(ostream file);


#endif