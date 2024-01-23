void example(char* a, char* b, char* c) {
#pragma HLS INTERFACE s_axilite port = a bundle = BUS_A offset = 0x20
#pragma HLS INTERFACE s_axilite port = b bundle = BUS_A offset = 0x28
#pragma HLS INTERFACE s_axilite port = c bundle = BUS_A offset = 0x30
#pragma HLS INTERFACE s_axilite port = return bundle = BUS_A

    *c += *a + *b;
}