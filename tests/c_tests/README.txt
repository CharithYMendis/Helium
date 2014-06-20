 halide_tests
 -------------
 
 
 asm_tests
 ----------
 
 
 c_tests
 ---------
 1. data_parallel -> 9 point blur stencil
 2. producer_consumer -> 1*3 point stencil -> 3*1 point stencil
 3. simple arithmetic expressions -> to test whether all are captured
 
 limitations
 ------------
 
 1. If some value is initialized inside the captured trace, the origin of that value may not be present in the final expression, but the substituted value
 2. Some iteration limits may not be inferred at all with in the expression