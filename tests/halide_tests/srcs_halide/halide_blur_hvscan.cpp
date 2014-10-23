#include <Halide.h>
#include <vector>


using namespace std;
using namespace Halide;


int main(int argc, char **argv) {

	ImageParam input(UInt(8), 2);
	Func blur_x("blur_x"), blur_y("blur_y"),clamped_x("clamped_y"), clamped_y("clamped_y");
	Var x("x"), y("y"), xi("xi"), yi("yi");

	clamped_x(x, y) = cast<uint16_t>(input(clamp(x, 1, input.width() - 1),y));

	// The algorithm
	blur_x(x, y) = (clamped_x(x-1, y) + clamped_x(x, y) + clamped_x(x + 1, y)) / 3;

	clamped_y(x, y) = blur_x(x, clamp(y, 1, input.height() - 1));

	blur_y(x, y) = cast<uint8_t>((clamped_y(x, y - 1) + clamped_y(x, y) + clamped_y(x, y + 1)) / 3);

	// How to schedule it
	blur_y.split(y, y, yi, 8).parallel(y).vectorize(x, 8);
	blur_x.store_at(blur_y, y).compute_at(blur_y, yi).vectorize(x, 8);

	blur_y.compile_to_file("halide_blur_hvscan_gen", input);

	return 0;
}

