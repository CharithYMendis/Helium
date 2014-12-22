#include <Halide.h>
#include <vector>
using namespace std;
using namespace Halide;
int main(){

	Var x_0;
	Var x_1;
	Var x_2;
	Param<double>  p_1("p_1");
	Param<double> input_2("input_2");
	ImageParam input_3(UInt(8), 3);
	Func output_1("output_1");
	//output_1(x_0, x_1, x_2) = cast<uint8_t>(((((cast<double>(input_3(x_0, x_1 + 1, x_2 + 1)) - (cast<double>(((cast<uint32_t>(input_3(x_0, x_1 + 1, x_2 + 2)) - (0 + (8 * cast<uint32_t>(input_3(x_0, x_1 + 1, x_2 + 1))) + 0)) + cast<uint32_t>(input_3(x_0, x_1 + 2, x_2 + 2)) + cast<uint32_t>(input_3(x_0, x_1, x_2 + 2)) + cast<uint32_t>(input_3(x_0, x_1 + 2, x_2 + 1)) + cast<uint32_t>(input_3(x_0, x_1, x_2 + 1)) + cast<uint32_t>(input_3(x_0, x_1 + 2, x_2)) + cast<uint32_t>(input_3(x_0, x_1 + 1, x_2)) + cast<uint32_t>(input_3(x_0, x_1, x_2)))) * cast<double>(p_1) * cast<double>(input_2)))))));
	
	output_1(x_0, x_1, x_2) = cast<uint8_t>(34);
	output_1.trace_stores(); 

	vector<Argument> args;
	args.push_back(p_1);
	args.push_back(input_2);
	args.push_back(input_3);
	output_1.compile_to_file("halide_misc_gen", args);

	
	return 0;
}
