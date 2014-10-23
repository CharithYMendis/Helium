#include <Halide.h>
#include <vector>


using namespace std;
using namespace Halide;


int main(){

	ImageParam input(UInt(8), 2);
	Func output("output"), clamped("clamped");
	Var x("x"), y("y");

	clamped(x, y) = cast<uint16_t>(input(clamp(x, 1, input.width() - 1), clamp(y, 1, input.height() - 1)));

	output(x, y) = cast<uint8_t>((clamped(x - 1, y - 1) + clamped(x, y - 1) + clamped(x + 1, y - 1)
		+ clamped(x - 1, y) + clamped(x, y) + clamped(x + 1, y)
		+ clamped(x - 1, y + 1) + clamped(x, y + 1) + clamped(x + 1, y + 1)) / 9);
	
	output.compile_to_file("halide_blur_naive_gen", input);
	
	return 0;
}

