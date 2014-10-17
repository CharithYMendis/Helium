#include <Windows.h>
#include "imageinfo.h"
#include "meminfo.h"
#include "imageinfo.h"
#include "utilities.h"
#include "common_defines.h"
#include <iostream>

using namespace std;

#define DIFF_MODE		1
#define TWO_IMAGE_MODE  2
#define ONE_IMAGE_MODE	3

bool debug = false;
uint32_t debug_level = 0;
ofstream log_file;

void create_arith_image(uint32_t width, uint32_t height, const char * name);
void create_row_image(uint32_t width, uint32_t height, const char * name);
void create_column_image(uint32_t width, uint32_t height, const char * name);


void print_usage(){
	printf("usage - format -<name> <value>\n");
	printf("\t width - width of the image \n");
	printf("\t height - height of the image \n");
}

int main(int argc, char **argv){

	string process_name(argv[0]);
	int width;
	int height;

	/*********************cmd line arg processing*************************/

	vector<cmd_args_t *> args = get_command_line_args(argc, argv);
	if (args.size() == 0){
		print_usage();
		exit(0);
	}

	for (int i = 0; i < args.size(); i++){
		if (args[i]->name.compare("-width") == 0){
			width = atoi(args[i]->value.c_str());
		}
		else if (args[i]->name.compare("-height") == 0){
			height = atoi(args[i]->value.c_str());
		}
		else if (args[i]->name.compare("-debug") == 0){
			debug = args[i]->value[0] - '0';
		}
		else if (args[i]->name.compare("-debug_level") == 0){
			debug_level = atoi(args[i]->value.c_str());
		}
		else{
			printf("unrecognized argument - %s\n", args[i]->name);
			exit(0);
		}
	}

	ULONG_PTR token = initialize_image_subsystem();

	string image_folder = get_standard_folder("image");

	Gdiplus::Bitmap * image = open_image( (image_folder + "\\low.png").c_str());
	byte * buffer = get_image_buffer(image);

		for (int j = 0; j < image->GetHeight(); j++){
			for (int i = 0; i < image->GetWidth(); i++){
			cout << (uint32_t)buffer[j * width + i] << endl;
		}
	}

	update_image_buffer(image, buffer);
	save_image(image, (image_folder + "\\test.png").c_str());
	delete image;

	create_column_image(width, height, (image_folder + "\\column.png").c_str());
	create_row_image(width, height, (image_folder + "\\row.png").c_str());
	create_arith_image(width, height, (image_folder + "\\arith.png").c_str());


	shutdown_image_subsystem(token);


	return 0;

}

void create_column_image(uint32_t width, uint32_t height, const char * name){

	Gdiplus::Bitmap * image = create_image(width, height);
	byte * buffer = get_image_buffer(image);

	for (int k = 0; k < 3; k++){
		for (int j = 0; j < height; j++){
			for (int i = 0; i < width; i++){
				uint32_t col = i;
				buffer[i + (j + k * height) * width] = col % 255;
			}
		}
	}

	update_image_buffer(image, buffer);

	save_image(image, name);
	delete image;


}

void create_row_image(uint32_t width, uint32_t height, const char * name){

	Gdiplus::Bitmap * image = create_image(width, height);
	byte * buffer = get_image_buffer(image);

	for (int k = 0; k < 3; k++){
		for (int j = 0; j < height; j++){
			for (int i = 0; i < width; i++){
				uint32_t row = j + 30;
				buffer[i + (j + k * height) * width] = (uint8_t)row % 255;
			}
		}
	}

	update_image_buffer(image, buffer);

	save_image(image, name);
	delete image;


}

void create_arith_image(uint32_t width, uint32_t height, const char * name){

	Gdiplus::Bitmap * image = create_image(width, height);
	byte * buffer = get_image_buffer(image);

	uint32_t count = 0;

	for (int j = 0; j < height; j++){
		for (int i = 0; i < width; i++){
			buffer[i + (j + 0 * height) * width] = count % 255;
			buffer[i + (j + 1 * height) * width] = count % 255;
			buffer[i + (j + 2 * height) * width] = count % 255;
			count++;
		}
	}

	update_image_buffer(image, buffer);

	save_image(image, name);
	delete image;


}











