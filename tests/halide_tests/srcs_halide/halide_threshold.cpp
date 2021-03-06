#include <Halide.h>
using namespace Halide;

int main(int argc, char **argv) {

    ImageParam input(UInt(8), 2);
    Func output("output");
    Var x("x"), y("y");
    
	uint8_t threshold = 128;

    // The algorithm
	output(x, y) = cast<uint8_t>(select(input(x, y) > threshold, 255, 0));
    output.compile_to_file("halide_threshold_gen", input); 

    return 0;
}
