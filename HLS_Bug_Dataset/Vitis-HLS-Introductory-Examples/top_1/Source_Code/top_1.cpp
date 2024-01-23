void top(char inval1, char inval2, char inval3, char* outval1, char* outval2,
         char* outval3) {
    *outval1 = foo(inval1, 0);
    *outval2 = foo(inval2, 1);
    *outval3 = foo(inval3, 100);
}

// Content of called function
char foo(char inval, char incr) {
#pragma HLS INLINE OFF
#pragma HLS FUNCTION_INSTANTIATE variable = incr
    return inval + incr;
}