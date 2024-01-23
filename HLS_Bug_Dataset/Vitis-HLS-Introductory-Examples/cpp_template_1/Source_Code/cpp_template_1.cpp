void cpp_template(int inp, int* out) {
    int out0, out1, out2, out3, out4;

    // Use templated functions to get multiple instances
    // of the same function.
    func_with_static<1>(inp, &out0);
    func_with_static<2>(inp, &out1);
    func_with_static<3>(inp, &out2);
    func_with_static<4>(inp, &out3);
    func_with_static<5>(inp, &out4);

    *out += out0 + out1 + out2 + out3 + out4;
}