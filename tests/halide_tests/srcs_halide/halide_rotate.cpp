#include <Halide.h>
using namespace Halide;

int main(int argc, char **argv) {

    ImageParam input(UInt(16), 2);
    Func rotate("rotate");
    Var x("x"), y("y"), xi("xi"), yi("yi");
    
    // The algorithm
    rotate(x,y) = input(y,x);
    
    rotate.compile_to_file("halide_rotate_gen", input); 

    return 0;
}
