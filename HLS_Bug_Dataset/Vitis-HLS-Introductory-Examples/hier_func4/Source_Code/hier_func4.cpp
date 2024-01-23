void hier_func4(din_t A, din_t B, dout_t* C, dout_t* D) {
    dint_t apb, amb;

    sumsub_func(&A, &B, &apb, &amb);
#ifndef __SYNTHESIS__
    FILE* fp1;
    char filename[255];
    sprintf(filename, "Out_apb_%03d.dat", apb);
    fp1 = fopen(filename, "w");
    fprintf(fp1, "%d \n", apb);
    fclose(fp1);
#endif
    shift_func(&apb, &amb, C, D);
}

// Content of called function
void sumsub_func(din_t* in1, din_t* in2, dint_t* outSum, dint_t* outSub) {
    *outSum = *in1 + *in2;
    *outSub = *in1 - *in2;
}

// Content of called function
void shift_func(dint_t* in1, dint_t* in2, dout_t* outA, dout_t* outB) {
    *outA = *in1 >> 1;
    *outB = *in2 >> 2;
}