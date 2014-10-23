#include <Halide.h>
using namespace Halide;

int main(int argc, char **argv) {

	ImageParam input(UInt(8), 2);
	Func output("output");
	Var x("x"), y("y");

	uint8_t scale = 2;

	// The algorithm
	output(x, y) = input(x / scale, y / scale);
	output.compile_to_file("halide_snakes_gen", input);

	return 0;
}
