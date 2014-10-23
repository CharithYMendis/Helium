#include <Halide.h>
using namespace Halide;

//this is going to a horizontal wrap to the right, can experiment with it.
int main(int argc, char **argv) {

    ImageParam input(UInt(8), 2);
    Func output("output");
	Var x("x"), y("y");

	uint8_t wrap_amount = 20;
	Expr wrap_index = select(x >= 20, x - 20, x - 20 + input.width());

    // The algorithm
	Func clamped;
	clamped(x, y) = input(clamp(x, 0, input.width() - 1),y);
	output(x, y) = clamped(x-20,y);
    output.compile_to_file("halide_wrap_gen", input); 

    return 0;
}
