#include <Halide.h>
using namespace Halide;

int main(int argc, char **argv) {

    ImageParam input(UInt(8), 3);
    Func rotate("rotate");
    Var x("x"), y("y"), xi("xi"), yi("yi"), c("c");
    
    // The algorithm
    rotate(x,y,c) = cast<unsigned char>(x % 255);

    
    rotate.compile_to_file("halide_rotate_gen", input); 

    return 0;
}
