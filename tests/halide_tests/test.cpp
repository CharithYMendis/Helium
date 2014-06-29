#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "static_image.h"

extern "C" {
#include "halide_blur.h"
}

Image<uint16_t> blur_halide(Image<uint16_t> in) {
    Image<uint16_t> out(in.width()-8, in.height()-2);

    // Call it once to initialize the halide runtime stuff
    halide_blur(in, out);

    // Compute the same region of the output as blur_fast (i.e., we're
    // still being sloppy with boundary conditions)
    halide_blur(in, out);


    return out;
}

int main(int argc, char **argv) {

    Image<uint16_t> input(6408, 4802);

    for (int y = 0; y < input.height(); y++) {
        for (int x = 0; x < input.width(); x++) {
            input(x, y) = rand() & 0xfff;
        }
    }

    Image<uint16_t> halide = blur_halide(input);

    return 0;
}
