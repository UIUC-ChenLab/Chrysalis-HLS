void hamming_window(int32_t outdata[WINDOW_LEN], int16_t indata[WINDOW_LEN]) {
  static int16_t window_coeff[WINDOW_LEN];
  unsigned i;

  // In order to ensure that 'window_coeff' is inferred and properly
  // initialized as a ROM, it is recommended that the array initialization
  // be done in a sub-function with global (wrt this source file) scope.
  hamming_rom_init(window_coeff);
process_frame:
  for (i = 0; i < WINDOW_LEN; i++) {
#pragma HLS pipeline
    outdata[i] = (int32_t)window_coeff[i] * (int32_t)indata[i];
  }
}