#include <Halide.h>
using namespace Halide;
#include <math.h>


int main(int argc, char **argv){

	ImageParam input(UInt(8), 2, "input");
	Var x("x"), y("y");
	Func output("output");

	//histogram equalization

	//1. calculate the histogram of original function
	RDom r(input);
	Func hist("hist");
	hist(x) = 0;
	hist(input(r.x, r.y)) = hist(input(r.x, r.y)) + 1; //serial dependancy

	//hist.trace_stores();

	//2. calculate cdf and normalize it
	RDom bins(1,256);
	Expr n = input.width() * input.height();
	hist(0) = hist(0);
	hist(bins) = (hist(bins) + hist(bins - 1));

	hist.compute_root();

	//hist.trace_stores();

	output(x, y) = cast<uint8_t>( (255 * hist(input(x, y))) / n);
	output.compile_to_file("halide_heq_gen", input);
	output.compile_to_lowered_stmt("hist.txt");

}
