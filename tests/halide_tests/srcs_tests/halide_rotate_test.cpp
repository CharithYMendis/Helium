#include <windows.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#include "image_manipulation.h"

extern "C" {
#include "halide_rotate_gen.h"
}

Image<uint16_t> halide_function(Image<uint16_t> in) {

    Image<uint16_t> out(in.height(), in.width());

    // Call it once to initialize the halide runtime stuff
    halide_rotate_gen(in, out);

    // Compute the same region of the output as blur_fast (i.e., we're
    // still being sloppy with boundary conditions)
    halide_rotate_gen(in, out);


    return out;
}

int main(int argc, char **argv) {

	ULONG_PTR token = initialize_image_subsystem();
	
	string name(argv[1]);
	
	Image<uint16_t> in = load_image<uint16_t>(argv[1]);
	
	
    Image<uint16_t> halide = halide_function(in);


	save_image<uint16_t>(argv[2], halide);

	shutdown_image_subsystem(token);

    return 0;
}
