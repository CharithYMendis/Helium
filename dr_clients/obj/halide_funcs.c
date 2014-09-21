typedef unsigned int bool;

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "halide_blur_gen.h"



/*
typedef struct buffer_t {
    uint64_t dev;
    uint8_t* host;
    int32_t extent[4];
    int32_t stride[4];
    int32_t min[4];
    int32_t elem_size;
    bool host_dirty;
    bool dev_dirty;
} buffer_t; */

typedef struct _dimension_t {

	uint32_t dims;
	uint32_t width;
	uint32_t height;
	uint32_t colors;
	
} dimension_t;

void setup_image(buffer_t * buf, uint8_t * host, dimension_t * bounds, uint32_t elem_size ){
	
	int i = 0;
	
	buf->extent[0] = bounds->width;
	buf->extent[1] = bounds->height;
	
	//printf("%u %u\n",buf->extent[0],buf->extent[1]);
	
	if(bounds->dims == 3){
		buf->extent[2] = bounds->colors;
	}
	else{
		buf->extent[2] = 0;
	}
	buf->extent[3] = 0;
	
	//printf("%u %u %u\n",buf->extent[0],buf->extent[1],buf->extent[2] );
	
	buf->stride[0] = 1;
	buf->stride[1] = bounds->width;
	if(bounds->dims == 3){
		buf->stride[2] = bounds->width * bounds->height;
	}
	else{
		buf->stride[2] = 0;
	}
	buf->stride[3] = 0;
	
	buf->elem_size = elem_size;
	buf->host = host;
	buf->host_dirty = 0;
	buf->dev_dirty = 0; 
	buf->dev = 0;
	
	for(i=0; i<4; i++){
		buf->min[i] = 0;
	}

}

void do_blur_test_run(unsigned int width, unsigned int height){

	unsigned char * input_image = (unsigned char *)malloc(sizeof(unsigned char)*width*height);
	unsigned char * output_image = (unsigned char *)malloc(sizeof(unsigned char)*width*height);

	buffer_t * input_buf = malloc(sizeof(buffer_t));
	buffer_t * output_buf = malloc(sizeof(buffer_t));
	
	input_buf->extent[0] = width; input_buf->extent[1] = height; input_buf->extent[2] = 0; input_buf->extent[3] = 0;
	input_buf->stride[0] = 1; input_buf->stride[1] = width; input_buf->stride[2] = 0; input_buf->stride[3] = 0;
	input_buf->min[0] = 0; input_buf->min[1] = 0; input_buf->min[2] = 0; input_buf->min[3] = 0;
	input_buf->elem_size = 1;
	input_buf->host = input_image;
	input_buf->host_dirty = 0;
	input_buf->dev_dirty = 0; 
	input_buf->dev = 0;
	
	output_buf->extent[0] = width - 2; output_buf->extent[1] = height - 2; output_buf->extent[2] = 0; output_buf->extent[3] = 0;
	output_buf->stride[0] = 1; output_buf->stride[1] = width; output_buf->stride[2] = 0; output_buf->stride[3] = 0;
	output_buf->min[0] = 1; output_buf->min[1] = 1; output_buf->min[2] = 0; output_buf->min[3] = 0;
	output_buf->elem_size = 1;
	output_buf->host = output_image + width + 1;
	output_buf->host_dirty = 0;
	output_buf->dev_dirty = 0; 
	output_buf->dev = 0;
	
	halide_blur_gen(input_buf, output_buf);

}


void do_blur_test(unsigned int width, unsigned int height, unsigned int input_image, unsigned int output_image){

	
	buffer_t * input_buf = malloc(sizeof(buffer_t));
	buffer_t * output_buf = malloc(sizeof(buffer_t));
	
	input_buf->extent[0] = width; input_buf->extent[1] = height; input_buf->extent[2] = 0; input_buf->extent[3] = 0;
	input_buf->stride[0] = 1; input_buf->stride[1] = width; input_buf->stride[2] = 0; input_buf->stride[3] = 0;
	input_buf->min[0] = 0; input_buf->min[1] = 0; input_buf->min[2] = 0; input_buf->min[3] = 0;
	input_buf->elem_size = 1;
	input_buf->host = input_image;
	input_buf->host_dirty = 0;
	input_buf->dev_dirty = 0; 
	input_buf->dev = 0;
	
	output_buf->extent[0] = width - 2; output_buf->extent[1] = height - 2; output_buf->extent[2] = 0; output_buf->extent[3] = 0;
	output_buf->stride[0] = 1; output_buf->stride[1] = width; output_buf->stride[2] = 0; output_buf->stride[3] = 0;
	output_buf->min[0] = 1; output_buf->min[1] = 1; output_buf->min[2] = 0; output_buf->min[3] = 0;
	output_buf->elem_size = 1;
	output_buf->host = output_image + width + 1;
	output_buf->host_dirty = 0;
	output_buf->dev_dirty = 0; 
	output_buf->dev = 0;
	
	halide_blur_gen(input_buf, output_buf);
}

void do_func_test(unsigned int width, unsigned int height, unsigned int input_image, unsigned int output_image){


	dimension_t * input_bounds = malloc(sizeof(dimension_t));
	input_bounds->dims = 2; input_bounds->width = width; input_bounds->height = height; input_bounds->colors = 1;

	dimension_t * output_bounds = malloc(sizeof(dimension_t));
	output_bounds->dims = 2; output_bounds->width = width; output_bounds->height = height; output_bounds->colors = 1;


	buffer_t * input_buf = malloc(sizeof(buffer_t));
	buffer_t * output_buf = malloc(sizeof(buffer_t));

	//printf("%x %x\n",input_image,output_image);
	
	setup_image(input_buf, (uint8_t *)input_image, input_bounds, 1);
	setup_image(output_buf, (uint8_t *)output_image, output_bounds, 1);

	halide_rotate_gen(input_buf, output_buf);
	
	//halide_rotate_gen(input_buf, output_buf);
}

void do_nothing_halide(){

	int x = 5 + 6;
	printf("hello\n");
	//uint8_t * ptr = malloc(sizeof(uint8_t) * 20);

}


/*int main(){

	uint8_t * input = malloc(144*120*3);
	uint8_t * output = malloc(144*120*3);
	int i=0;
	int j=0;
	
	for(j=0; j<120; j++){
		for(i=0; i<144; i++){
			input[i + (j + 0 * 120) * 144] = i % 255;
			input[i + (j + 1 * 120) * 144] = i % 255;
			input[i + (j + 2 * 120) * 144] = i % 255;
			output[i + (j + 0 * 120) * 144] = 0;
			output[i + (j + 1 * 120) * 144] = 0;
			output[i + (j + 2 * 120) * 144] = 0;
		}
	}
	
	printf("%x %x\n",input,output);
	
	do_func_test(input,output);
	
	for(i=0; i<10; i++){
		printf("%u %u\n",input[i],output[i]);
	}
	
	do_blur_test_run(100,100);


}*/







































