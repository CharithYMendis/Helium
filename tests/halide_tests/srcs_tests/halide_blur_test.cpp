#include <windows.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#include "image_manipulation.h"

extern "C" {
#include "halide_blur_gen.h"
}

Image<uint16_t> blur_halide(Image<uint16_t> in) {

    Image<uint16_t> out(in.width()-8, in.height()-2);

    // Call it once to initialize the halide runtime stuff
    halide_blur_gen(in, out);

    // Compute the same region of the output as blur_fast (i.e., we're
    // still being sloppy with boundary conditions)
	
    halide_blur_gen(in, out);


    return out;
}

int main(int argc, char **argv) {

	ULONG_PTR token = initialize_image_subsystem();
	
	string name(argv[1]);
	
	Image<uint16_t> ppm_in = load_image<uint16_t>(argv[1]);
	
	
    Image<uint16_t> halide = blur_halide(ppm_in);

	save_image<uint16_t>(argv[2], halide);

	shutdown_image_subsystem(token);

    return 0;
}
