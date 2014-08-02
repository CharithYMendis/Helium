#include <Halide.h>
using namespace Halide;
#include <math.h>

Expr weight(Expr x, Expr y){

	uint16_t max_value = (uint16_t)((~0));

	return exp(-((x - y) * (x - y)) / (2 * (max_value*0.1)*(max_value*0.1)));
}


int main(int argc, char **argv) {

    ImageParam input(UInt(16), 2);
    Func output("output");
    Var x("x"), y("y"), xi("xi"), yi("yi");
	
	Expr total_weight = weight(input(x+1,y),input(x,y)) + weight(input(x+2,y),input(x,y)); 
	
	Func input_8;
	input_8(x, y) = cast<int16_t>(input(x, y));
    
    // The algorithm
    output(x,y) =  (input(x,y) + weight(input(x+1,y),input(x,y))*input(x+1,y) + weight(input(x-1,y),input(x,y))*input(x-1,y))/total_weight;
	//output(x, y) = weight(input_8(x+1, y),input_8(x,y));
	//output.trace_stores();

    output.compile_to_file("halide_input_dep_gen", input); 

    return 0;
}
