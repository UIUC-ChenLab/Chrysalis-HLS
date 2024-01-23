void read_sine_lut( lut_word_t cos_lut[LUTSIZE], const int LUTSIZE ) {



  lut_adr_t i;            // cover full quadrant

  quad_adr_t lsb,adr;     // cover 1/4 quadrant

  ap_uint<2>  msb;        // specify which quadrant

  lut_word_t  lut_word; 



//  ofstream fp_dout ("fullsine.txt");



  for (int k=0;k<4*LUTSIZE;k++) {



    i    = k;

    msb  = i(11,10);

    lsb  = i(9,0);



    if (msb==1) {         // left top

       adr      = lsb;

       lut_word = cos_lut[adr];

    } else if (msb==2) {  // left bot 

       if (lsb==0) lut_word = 0;

       else { 

         adr      = -lsb;

         lut_word = -cos_lut[adr];

       }

    } else if (msb==0) {  // right top

       if (lsb==0) lut_word = 0;

       else { 

         adr      = -lsb;

         lut_word =  cos_lut[adr];

       }

    } else             {  // right bot 

         adr      =  lsb;

         lut_word = -cos_lut[adr];

    }



//    fp_dout << scientific << lut_word << endl;



  }



}