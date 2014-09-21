#ifndef _HALIDE_EXALGO_H
#define _HALIDE_EXALGO_H

void do_blur_test_run(unsigned int width, unsigned int height);
void do_blur_test(unsigned int width, unsigned int height, unsigned int input_image, unsigned int output_image);
void do_func_test(unsigned int width, unsigned int height, unsigned int input_image, unsigned int output_image);
void do_nothing_halide();

#endif