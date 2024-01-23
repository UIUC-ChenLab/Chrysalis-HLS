dout_t cpp_ap_fixed(din1_t d_in1, din2_t d_in2) {

    static dint_t sum;
    sum = +d_in1;
    return sum * d_in2;
}