#include <windows.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#include "image_manipulation.h"

extern "C" {
#include "halide_misc_gen.h"
}


Image<uint8_t> halide_function(Image<uint8_t> in) {

	Image<uint8_t> out(in.width(), in.height());

	// Call it once to initialize the halide runtime stuff
	halide_misc_gen(in, out);

	return out;
}

int main(int argc, char **argv) {

	ULONG_PTR token = initialize_image_subsystem();

	string name(argv[1]);
	Image<uint8_t> input = load_image<uint8_t>(argv[1]);
	Image<uint8_t> output = halide_function(input);
	save_image<uint8_t>(argv[2], output);

	shutdown_image_subsystem(token);

	return 0;
}