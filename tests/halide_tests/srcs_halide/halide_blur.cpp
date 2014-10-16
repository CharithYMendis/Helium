/*#include <Halide.h>
using namespace Halide;

int main(int argc, char **argv) {

    ImageParam input(UInt(8), 2);
    Func blur_x("blur_x"), blur_y("blur_y");
    Var x("x"), y("y"), xi("xi"), yi("yi");
    
    // The algorithm
    //blur_x(x, y) = (input(x-1, y) + input(x, y) + input(x+1, y))/3;
    //blur_y(x, y) = ((blur_x(x, y-1) + blur_x(x, y) + blur_x(x, y+1))/3);
	

	//blur_y(x, y) = cast<uint8_t>((cast<uint16_t>(input(x - 1, y)) + cast<uint16_t>(input(x + 1, y)) + cast<uint16_t>(input(x, y - 1))
	//	+ cast<uint16_t>(input(x, y + 1)) + 4 * cast<uint16_t>(input(x, y))) / 8);




	//blur_y(x, y) = cast<uint8_t>(255);

	//blur_y(x, y) = input(x, y);

    // How to schedule it
    //blur_y.split(y, y, yi, 8).parallel(y).vectorize(x, 8);
    //blur_x.store_at(blur_y, y).compute_at(blur_y, yi).vectorize(x, 8);  
    
    blur_y.compile_to_file("halide_blur_gen", input); 

    return 0;
}*/

#include <Halide.h>
#include <vector>
using namespace std;
using namespace Halide;
int main(){

	Var x_0;
	Var x_1;
	ImageParam input_1(UInt(8), 2);
	Func output_2("output_2");
	output_2(x_0, x_1) = cast<uint8_t>(((((4 + (4 * cast<uint32_t>(input_1(x_0 + 1, x_1 + 1))) + cast<uint32_t>(input_1(x_0 + 1, x_1)) + cast<uint32_t>(input_1(x_0, x_1 + 1)) + cast<uint32_t>(input_1(x_0 + 2, x_1 + 1)) + cast<uint32_t>(input_1(x_0 + 1, x_1 + 2))) >> cast<uint32_t>(3))) & 255));
	vector<Argument> args;
	args.push_back(input_1);
	output_2.parallel(x_1,16).vectorize(x_0, 16);
	output_2.compile_to_file("halide_blur_gen", args);

	
	return 0;
}

