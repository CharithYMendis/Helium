#include <Halide.h>
using namespace Halide;
#include <vector>

using namespace std;

int main(int argc, char **argv) {

		/*Var x_0;
		Var x_1;
		Var x_2;
		Param<double> p_1("p_1");
		Param<double> input_3("input_3");
		Param<double> input_2("input_2");
		Param<double> input_5("input_5");
		ImageParam input_4(UInt(8), 3);
		Func output_1;
		output_1(x_0, x_1, x_2) = cast<uint8_t>((((((cast<double>((cast<uint32_t>(input_4(x_0, x_1 + 2, x_2 + 2)) + cast<uint32_t>(input_4(x_0, x_1 + 1, x_2 + 2)) + cast<uint32_t>(input_4(x_0, x_1, x_2 + 2)) + cast<uint32_t>(input_4(x_0, x_1 + 2, x_2 + 1)) + cast<uint32_t>(input_4(x_0, x_1, x_2 + 1)) + cast<uint32_t>(input_4(x_0, x_1 + 2, x_2)) + cast<uint32_t>(input_4(x_0, x_1 + 1, x_2)) + cast<uint32_t>(input_4(x_0, x_1, x_2)))) * cast<double>(p_1) * cast<double>(input_5) * cast<double>(input_2)) + ((cast<double>(input_3) -(cast<double>(p_1) * cast<double>(input_2))) * cast<double>(input_4(x_0, x_1 + 1, x_2 + 1))))))));
		vector<Argument> args;
		args.push_back(p_1);
		args.push_back(input_3);
		args.push_back(input_2);
		args.push_back(input_5);
		args.push_back(input_4);*/

	Var x_0;
	Var x_1;
	Var x_2;
	Param<double>  p_1("p_1");
	Param<double> input_2("input_2");
	ImageParam input_3(UInt(8), 3);
	Func output_1;
	Expr val = (((((cast<double>(input_3(x_0, x_1 + 1, x_2 + 1)) - (cast<double>(((cast<double>(input_3(x_0, x_1 + 1, x_2 + 2)) - (0 + (8 * cast<double>(input_3(x_0, x_1 + 1, x_2 + 1))) + 0)) + cast<double>(input_3(x_0, x_1 + 2, x_2 + 2)) + cast<double>(input_3(x_0, x_1, x_2 + 2)) + cast<double>(input_3(x_0, x_1 + 2, x_2 + 1)) + cast<double>(input_3(x_0, x_1, x_2 + 1)) + cast<double>(input_3(x_0, x_1 + 2, x_2)) + cast<double>(input_3(x_0, x_1 + 1, x_2)) + cast<double>(input_3(x_0, x_1, x_2)))) * cast<double>(p_1) * cast<double>(input_2)))))));
	//output_1(x_0, x_1, x_2) = cast<uint8_t>(((((cast<double>(input_3(x_0, x_1 + 1, x_2 + 1)) - (cast<double>(((cast<double>(input_3(x_0, x_1 + 1, x_2 + 2)) - (0 + (8 * cast<double>(input_3(x_0, x_1 + 1, x_2 + 1))) + 0)) + cast<double>(input_3(x_0, x_1 + 2, x_2 + 2)) + cast<double>(input_3(x_0, x_1, x_2 + 2)) + cast<double>(input_3(x_0, x_1 + 2, x_2 + 1)) + cast<double>(input_3(x_0, x_1, x_2 + 1)) + cast<double>(input_3(x_0, x_1 + 2, x_2)) + cast<double>(input_3(x_0, x_1 + 1, x_2)) + cast<double>(input_3(x_0, x_1, x_2)))) * cast<double>(p_1) * cast<double>(input_2)))))));
	
	output_1(x_0, x_1, x_2) = cast<uint8_t>(clamp(val,0,255));
	
	vector<Argument> args;
	args.push_back(p_1);
	args.push_back(input_2);
	args.push_back(input_3);

	Halide::Var _x_11, _x_22;
	output_1
		.split(x_1, x_1, _x_11, 2)
		.split(x_2, x_2, _x_22, 2)
		.reorder(_x_11, x_1, _x_22, x_0, x_2)
		.reorder_storage(x_0, x_1, x_2)
		.parallel(x_2)
		.compute_root()
		; 
	

	/*Halide::Var _x_22;
output_1
.split(x_2, x_2, _x_22, 2)
.reorder(x_1, _x_22, x_0, x_2)
.reorder_storage(x_2, x_0, x_1)
.parallel(x_2)
.compute_root()
;*/

    output_1.compile_to_file("halide_rotate_gen", args); 


    return 0;
}
