#include <windows.h>
#include <string>
#include <gdiplus.h>
#include <iostream>
#include <stdint.h>
#include <wchar.h>
#include <assert.h>
#include "image_manipulation.h"

using namespace std;
using namespace Gdiplus;

void inverse(byte * input, byte * output, int height, int width, int fields){

	for (int i = 0; i < height; i++){
		for (int j = 0; j < width; j++){
			uint32_t index = (i * width + j)*fields;

			for (int k = 0; k < fields; k++){
				output[index + k] = 255 - input[index + k];
			}

		}
	}


}

int main(int argc, char* argv[]){


	ULONG_PTR token = initialize_image_subsystem();

	assert(argc == 3);

	wchar_t * input_file = new wchar_t[strlen(argv[1] + 1)];
	wchar_t * output_file = new wchar_t[strlen(argv[2] + 1)];
	mbstowcs(input_file, argv[1], strlen(argv[1]) + 1);
	mbstowcs(output_file, argv[2], strlen(argv[2]) + 1);


	uint32_t width;
	uint32_t height;
	uint32_t fields;

	Bitmap * image = Bitmap::FromFile(input_file);

	byte * input_buffer = get_image_buffer(image, &height, &width, &fields);

	/* for inverse we need the same sized array as input_buffer */
	byte * output_buffer = new byte[height * width * fields];

	inverse(input_buffer, output_buffer, height, width, 3);

	update_image_buffer(image, output_buffer);

	save_image(image, output_file);


	delete image;

	shutdown_image_subsystem(token);

	return 0;
}



