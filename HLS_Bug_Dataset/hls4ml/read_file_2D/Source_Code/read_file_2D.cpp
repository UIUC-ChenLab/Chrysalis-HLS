int read_file_2D(const char *filename, dataType data[nrows][ncols]) {
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == 0) {
        return -1;
    }
    // Read data from file
    float newval;
    for (int ii = 0; ii < nrows; ii++) {
        for (int jj = 0; jj < ncols; jj++) {
            if (fscanf(fp, "%f\n", &newval) != 0) {
                data[ii][jj] = newval;
            } else {
                return -2;
            }
        }
    }
    fclose(fp);
    return 0;
}