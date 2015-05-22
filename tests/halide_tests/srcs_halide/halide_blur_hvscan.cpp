#include <Halide.h>
#include <vector>


using namespace std;
using namespace Halide;


int main(int argc, char **argv) {

	ImageParam input(UInt(8), 2);
	
	Func blur_x("blur_x"), blur_y("blur_y");
	Var x("x"), y("y"), xi("xi"), yi("yi"), xo("xo"), yo("yo");
	Var x_outer, y_outer, x_inner, y_inner, tile_index;
	

	// The algorithm
	
	//1. one buffer
	/*blur_y(x, y) = cast<uint8_t>((cast<uint16_t>(input(x, y)) + cast<uint16_t>(input(x + 1, y)) + cast<uint16_t>(input(x + 2, y)) +
	cast<uint16_t>(input(x, y + 1)) + cast<uint16_t>(input(x + 1, y + 1)) + cast<uint16_t>(input(x + 2, y + 1)) +
	cast<uint16_t>(input(x, y + 2)) + cast<uint16_t>(input(x + 1, y + 2)) + cast<uint16_t>(input(x + 2, y + 2))) / 9);
	*/
	
	//2. two buffers
	blur_x(x, y) = (cast<uint16_t>(input(x, y)) + cast<uint16_t>(input(x + 1, y)) + cast<uint16_t>(input(x + 2, y))) / 3;
	blur_y(x, y) = cast<uint8_t>((blur_x(x, y) + blur_x(x, y + 1) + blur_x(x, y + 2)) / 3);

	

	// How to schedule it
	//blur_y.split(y, y, yi, 8).parallel(y).vectorize(x, 8);
	//blur_x.store_at(blur_y, y).compute_at(blur_y, yi).vectorize(x, 8);

	//blur_y.split(y, y, yi, 8).parallel(y);
	//blur_x.store_at(blur_y, y).compute_at(blur_y, yi);



	//blur_y.vectorize(x, 8);

	
	//blur_y.split(y, y, yi, 2);
	//blur_x.store_at(blur_y, y).compute_at(blur_y, yi);
	//blur_y.unroll(x, 2);

	//blur_y.tile(x, y, x_outer, y_outer, x_inner, y_inner, 2, 2);
	//blur_y.fuse(x_outer, y_outer, tile_index);
	//blur_y.parallel(tile_index);

	
	blur_y.compile_to_file("halide_blur_hvscan_gen", input);

	//7. threshold
	/*Var x_0;
	Var x_1;
	Var x_2;
	Var x_3;
	ImageParam input_2(UInt(8), 2);
	ImageParam input_1(UInt(8), 2);
	ImageParam input_3(UInt(8), 2);
	Param<uint32_t> thresh;
	Func inter_2;

	Expr inter_2_p__1 = ((0 & 1) - 1);
	Expr inter_2_p__0 = select((cast<uint32_t>(((((8192 + (4915 * cast<uint32_t>(input_2(x_0, x_1))) + (9667 * cast<uint32_t>(input_1(x_0, x_1))) + (1802 * cast<uint32_t>(input_3(x_0, x_1)))) >> cast<uint32_t>(14))) & 255)) < cast<uint32_t>(((thresh) & 255))), (((0 - 1) & 1) - 1), inter_2_p__1);
	inter_2(x_0, x_1) = cast<uint8_t>(clamp(inter_2_p__0, 0, 255));

	vector<Argument> arguments;
	arguments.push_back(input_2);;
	arguments.push_back(input_1);;
	arguments.push_back(input_3);;
	arguments.push_back(thresh);;
	inter_2.compile_to_file("halide_blur_hvscan_gen", arguments);
	return 0;
	*/

	//8. brightness


	/*ImageParam input(UInt(8), 2);
	Param<double> alpha;
	Param<double> beta;
	Var x("x"), y("y");

	Func output;

	output(x, y) = cast<uint8_t>(alpha * input(x, y) + beta);

	vector<Argument> arguments;
	arguments.push_back(input);;
	arguments.push_back(alpha);;
	arguments.push_back(beta);;
	output.compile_to_file("halide_blur_hvscan_gen", arguments);
	*/

	/*ImageParam input(UInt(8), 2);
	Var x("x"), y("y");

	Func output;

	output(x, y) = cast<uint8_t>(clamp((-cast<int32_t>(input(x, y)) - cast<int32_t>(input(x + 1, y)) - cast<int32_t>(input(x + 2, y)) 
		- cast<int32_t>(input(x, y + 1)) - cast<int32_t>(input(x + 2, y + 1)) - cast<int32_t>(input(x + 2, y + 2)) 
		- cast<int32_t>(input(x + 1, y + 2)) - cast<int32_t>(input(x, y + 2)) 
		+ 8 * cast<int32_t>(input(x + 1, y + 1))), 0, 255)); 

	//output(x, y) = cast<uint8_t>((cast<uint16_t>(input(x, y)) + cast<uint16_t>(input(x + 1, y + 1)) + cast<uint16_t>(input(x + 2, y + 2))) / 3);


	output(x, y) = select(input(x, y) > input(x + 1, y) && input(x, y) > input(x + 2, y), input(x, y)/2,
		select(input(x + 1, y) > input(x, y) && input(x + 1, y) > input(x + 2, y), input(x + 1, y)*2, input(x + 2, y)));

	vector<Argument> arguments;
	arguments.push_back(input);;
	output.compile_to_file("halide_blur_hvscan_gen", arguments);*/

	return 0;




}

