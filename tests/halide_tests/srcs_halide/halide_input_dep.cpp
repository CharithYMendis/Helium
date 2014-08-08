#include <Halide.h>
using namespace Halide;
#include <math.h>

Expr weight(Expr x, Expr y){

	uint16_t max_value = 255;

	Expr ret = exp(-((x - y) * (x - y)) / (2 * (max_value * float(0.1)) * (max_value * float(0.1))));

	return cast<uint16_t>(ret);
}


int main(int argc, char **argv) {

    ImageParam input(UInt(16), 2);
    Func output("output");
	Var x("x"), y("y");


	Expr total_weight = weight(input(x + 1, y), input(x, y)) + weight(input(x + 2, y), input(x, y));

	output(x,y) = select(total_weight == 0, input(x, y), (input(x, y) + weight(input(x + 1, y), input(x, y))*input(x + 1, y) + weight(input(x + 2, y), input(x, y))*input(x + 2, y)) / total_weight);
	output.trace_loads();
	output.trace_stores();

    output.compile_to_file("halide_input_dep_gen", input); 

    return 0;
}
