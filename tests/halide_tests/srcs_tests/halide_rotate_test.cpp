#include <windows.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#include "image_manipulation.h"

extern "C" {
#include "halide_rotate_gen.h"
}

Image<uint8_t> halide_function(Image<uint8_t> in) {

	Image<uint8_t> out(3, in.height() - 2, in.channels() - 2);
	//cout << in.channels() << endl;
	//cout << in.height() << endl;
	//cout << in.width() << endl;

	// Call it once to initialize the halide runtime stuff
	halide_rotate_gen(20, 5e-3, in, out);

	//const double _p_1, const double _input_3, const double _input_2, const double _input_5,
	//halide_rotate_gen(33.0, 1.0, 1e-2, 1.25e-1 , in, out);

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
