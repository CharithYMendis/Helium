#include <Halide.h>
#include <windows.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include "image_manipulation_halide.h"

using namespace std;


int main(int argc, char **argv) {

	ULONG_PTR token = initialize_image_subsystem();
	string name(argv[1]);

	Image<uint8_t> input = load_image<uint8_t>(argv[1]);
	Var x("x"), y("y"), c("c");
	Func output("output");

	cout << input.width() << " " << input.height() << " " << input.channels() << endl;

	//1. calculate the histogram of original function
	RDom r(input);
	Func hist("hist");
	hist(x) = 0;
	hist(input(r.x, r.y, r.z)) = hist(input(r.x, r.y, r.z)) + 1; //serial dependancy


	//2. calculate cdf and normalize it (min,extent)
	RDom bins(1, 255);
	Expr n = input.width() * input.height() * input.channels();
	hist(bins) = (hist(bins) + hist(bins - 1));

	/*Image<int32_t> hist_image = hist.realize(256);
	for (int i = 0; i < 256; i++){
	cout << hist_image(i) << endl;
	}*/
	
	hist.compute_root();

	//hist.trace_stores();
	output(x, y, c) = cast<uint8_t>((255 * hist(input(x, y, c))) / n);
	//output.vectorize(x, 4).parallel(y);
	//output.trace_stores();
	output.parallel(y);

	Image<uint8_t>  out_image = output.realize(input.width(), input.height(), input.channels());


	save_image<uint8_t>(argv[2], out_image);
	shutdown_image_subsystem(token);

	return 0;

}

