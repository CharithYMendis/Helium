#include <Halide.h>
using namespace Halide;

int main(int argc, char **argv) {

    ImageParam input(UInt(8), 2);
    Func blur_x("blur_x"), blur_y("blur_y");
    Var x("x"), y("y"), xi("xi"), yi("yi");
    
    // The algorithm
    //blur_x(x, y) = (input(x-1, y) + input(x, y) + input(x+1, y))/3;
    //blur_y(x, y) = ((blur_x(x, y-1) + blur_x(x, y) + blur_x(x, y+1))/3);
	

	blur_y(x, y) = cast<uint8_t>((cast<uint16_t>(input(x - 1, y)) + cast<uint16_t>(input(x + 1, y)) + cast<uint16_t>(input(x, y - 1))
		+ cast<uint16_t>(input(x, y + 1)) + 4 * cast<uint16_t>(input(x, y))) / 8);

	//blur_y(x, y) = cast<uint8_t>(255);

	//blur_y(x, y) = input(x, y);

    // How to schedule it
    //blur_y.split(y, y, yi, 8).parallel(y).vectorize(x, 8);
    //blur_x.store_at(blur_y, y).compute_at(blur_y, yi).vectorize(x, 8);  
    
    blur_y.compile_to_file("halide_blur_gen", input); 

    return 0;
}
