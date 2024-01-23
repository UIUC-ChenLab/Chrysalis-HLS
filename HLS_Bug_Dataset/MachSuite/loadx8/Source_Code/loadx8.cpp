void loadx8(TYPE a_x[], TYPE x[], int offset, int sx){
    a_x[0] = x[0*sx+offset];
    a_x[1] = x[1*sx+offset];
    a_x[2] = x[2*sx+offset];
    a_x[3] = x[3*sx+offset];
    a_x[4] = x[4*sx+offset];
    a_x[5] = x[5*sx+offset];
    a_x[6] = x[6*sx+offset];
    a_x[7] = x[7*sx+offset];
}