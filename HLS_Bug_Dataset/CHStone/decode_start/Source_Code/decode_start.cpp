void
decode_start (int *out_data_image_width, int *out_data_image_height,
	      int *out_data_comp_vpos, int *out_data_comp_hpos)
{
  int i;
  int CurrentMCU = 0;
  int HuffBuff[NUM_COMPONENT][DCTSIZE2];
  int IDCTBuff[6][DCTSIZE2];

  /* Read buffer */
  CurHuffReadBuf = p_jinfo_jpeg_data;

  /*
   * Initial value of DC element is 0
   */
  for (i = 0; i < NUM_COMPONENT; i++)
    {
      HuffBuff[i][0] = 0;
    }

  /*
   * Set the size of image to output buffer
   */
  *out_data_image_width = p_jinfo_image_width;
  *out_data_image_height = p_jinfo_image_height;

  /*
   * Initialize output buffer
   */
  for (i = 0; i < RGB_NUM; i++)
    {
      out_data_comp_vpos[i] = 0;
      out_data_comp_hpos[i] = 0;
    }


  if (p_jinfo_smp_fact == SF1_1_1)
    {
      printf ("Decode 1:1:1 NumMCU = %d\n", p_jinfo_NumMCU);

      /*
       * 1_1_1
       */
      while (CurrentMCU < p_jinfo_NumMCU)
	{

	  for (i = 0; i < NUM_COMPONENT; i++)
	    {
	      decode_block (i, IDCTBuff[i], HuffBuff[i]);
	    }


	  YuvToRgb (0, IDCTBuff[0], IDCTBuff[1], IDCTBuff[2]);
	  /*
	   * Write
	   */
	  for (i = 0; i < RGB_NUM; i++)
	    {
	      WriteBlock (&rgb_buf[0][i][0],
			  &out_data_comp_vpos[i],
			  &out_data_comp_hpos[i], &OutData_comp_buf[i][0]);
	    }
	  CurrentMCU++;
	}

    }
  else
    {
      printf ("Decode 4:1:1 NumMCU = %d\n", p_jinfo_NumMCU);
      /*
       * 4_1_1
       */
      while (CurrentMCU < p_jinfo_NumMCU)
	{
	  /*
	   * Decode Y element
	   * Decoding Y, U and V elements should be sequentially conducted for the use of Huffman table
	   */

	  for (i = 0; i < 4; i++)
	    {
	      decode_block (0, IDCTBuff[i], HuffBuff[0]);
	    }

	  /* Decode U */
	  decode_block (1, IDCTBuff[4], HuffBuff[1]);

	  /* Decode V */
	  decode_block (2, IDCTBuff[5], HuffBuff[2]);


	  /* Transform from Yuv into RGB */

	  for (i = 0; i < 4; i++)
	    {
	      YuvToRgb (i, IDCTBuff[i], IDCTBuff[4], IDCTBuff[5]);
	    }


	  for (i = 0; i < RGB_NUM; i++)
	    {
	      Write4Blocks (&rgb_buf[0][i][0],
			    &rgb_buf[1][i][0],
			    &rgb_buf[2][i][0],
			    &rgb_buf[3][i][0],
			    &out_data_comp_vpos[i],
			    &out_data_comp_hpos[i], &OutData_comp_buf[i][0]);
	    }


	  CurrentMCU += 4;
	}
    }
}

// Content of called function
void
decode_block (int comp_no, int *out_buf, int *HuffBuff)
{
  int QuantBuff[DCTSIZE2];
  unsigned int *p_quant_tbl;

  DecodeHuffMCU (HuffBuff, comp_no);

  IZigzagMatrix (HuffBuff, QuantBuff);

  p_quant_tbl =
    &p_jinfo_quant_tbl_quantval[(int)p_jinfo_comps_info_quant_tbl_no[comp_no]][DCTSIZE2];
  IQuantize (QuantBuff, p_quant_tbl);

  ChenIDct (QuantBuff, out_buf);

  PostshiftIDctMatrix (out_buf, IDCT_SHIFT);

  BoundIDctMatrix (out_buf, IDCT_BOUNT);

}

// Content of called function
#define c7d16 100L

#define c5d16 284L

#define c3d16 426L


#define c1d16 502L

#define c3d8 196L


#define c1d8 473L


#define c1d4 362L

void
ChenIDct (int *x, int *y)
{
  register int i;
  register int *aptr;
  register int a0, a1, a2, a3;
  register int b0, b1, b2, b3;
  register int c0, c1, c2, c3;

  /* Loop over columns */

  for (i = 0; i < 8; i++)
    {
      aptr = x + i;
      b0 = LS (*aptr, 2);
      aptr += 8;
      a0 = LS (*aptr, 2);
      aptr += 8;
      b2 = LS (*aptr, 2);
      aptr += 8;
      a1 = LS (*aptr, 2);
      aptr += 8;
      b1 = LS (*aptr, 2);
      aptr += 8;
      a2 = LS (*aptr, 2);
      aptr += 8;
      b3 = LS (*aptr, 2);
      aptr += 8;
      a3 = LS (*aptr, 2);

      /* Split into even mode  b0 = x0  b1 = x4  b2 = x2  b3 = x6.
         And the odd terms a0 = x1 a1 = x3 a2 = x5 a3 = x7.
       */

      c0 = MSCALE ((c7d16 * a0) - (c1d16 * a3));
      c1 = MSCALE ((c3d16 * a2) - (c5d16 * a1));
      c2 = MSCALE ((c3d16 * a1) + (c5d16 * a2));
      c3 = MSCALE ((c1d16 * a0) + (c7d16 * a3));

      /* First Butterfly on even terms. */

      a0 = MSCALE (c1d4 * (b0 + b1));
      a1 = MSCALE (c1d4 * (b0 - b1));

      a2 = MSCALE ((c3d8 * b2) - (c1d8 * b3));
      a3 = MSCALE ((c1d8 * b2) + (c3d8 * b3));

      b0 = a0 + a3;
      b1 = a1 + a2;
      b2 = a1 - a2;
      b3 = a0 - a3;

      /* Second Butterfly */

      a0 = c0 + c1;
      a1 = c0 - c1;
      a2 = c3 - c2;
      a3 = c3 + c2;

      c0 = a0;
      c1 = MSCALE (c1d4 * (a2 - a1));
      c2 = MSCALE (c1d4 * (a2 + a1));
      c3 = a3;

      aptr = y + i;
      *aptr = b0 + c3;
      aptr += 8;
      *aptr = b1 + c2;
      aptr += 8;
      *aptr = b2 + c1;
      aptr += 8;
      *aptr = b3 + c0;
      aptr += 8;
      *aptr = b3 - c0;
      aptr += 8;
      *aptr = b2 - c1;
      aptr += 8;
      *aptr = b1 - c2;
      aptr += 8;
      *aptr = b0 - c3;
    }

  /* Loop over rows */

  for (i = 0; i < 8; i++)
    {
      aptr = y + LS (i, 3);
      b0 = *(aptr++);
      a0 = *(aptr++);
      b2 = *(aptr++);
      a1 = *(aptr++);
      b1 = *(aptr++);
      a2 = *(aptr++);
      b3 = *(aptr++);
      a3 = *(aptr);

      /*
         Split into even mode  b0 = x0  b1 = x4  b2 = x2  b3 = x6.
         And the odd terms a0 = x1 a1 = x3 a2 = x5 a3 = x7.
       */

      c0 = MSCALE ((c7d16 * a0) - (c1d16 * a3));
      c1 = MSCALE ((c3d16 * a2) - (c5d16 * a1));
      c2 = MSCALE ((c3d16 * a1) + (c5d16 * a2));
      c3 = MSCALE ((c1d16 * a0) + (c7d16 * a3));

      /* First Butterfly on even terms. */

      a0 = MSCALE (c1d4 * (b0 + b1));
      a1 = MSCALE (c1d4 * (b0 - b1));

      a2 = MSCALE ((c3d8 * b2) - (c1d8 * b3));
      a3 = MSCALE ((c1d8 * b2) + (c3d8 * b3));

      /* Calculate last set of b's */

      b0 = a0 + a3;
      b1 = a1 + a2;
      b2 = a1 - a2;
      b3 = a0 - a3;

      /* Second Butterfly */

      a0 = c0 + c1;
      a1 = c0 - c1;
      a2 = c3 - c2;
      a3 = c3 + c2;

      c0 = a0;
      c1 = MSCALE (c1d4 * (a2 - a1));
      c2 = MSCALE (c1d4 * (a2 + a1));
      c3 = a3;

      aptr = y + LS (i, 3);
      *(aptr++) = b0 + c3;
      *(aptr++) = b1 + c2;
      *(aptr++) = b2 + c1;
      *(aptr++) = b3 + c0;
      *(aptr++) = b3 - c0;
      *(aptr++) = b2 - c1;
      *(aptr++) = b1 - c2;
      *(aptr) = b0 - c3;
    }

  /*
     Retrieve correct accuracy. We have additional factor
     of 16 that must be removed.
   */

  for (i = 0, aptr = y; i < 64; i++, aptr++)
    *aptr = (((*aptr < 0) ? (*aptr - 8) : (*aptr + 8)) / 16);
}

// Content of called function
void
BoundIDctMatrix (int *matrix, int Bound)
{
  int *mptr;

  for (mptr = matrix; mptr < matrix + DCTSIZE2; mptr++)
    {
      if (*mptr < 0)
	{
	  *mptr = 0;
	}
      else if (*mptr > Bound)
	{
	  *mptr = Bound;
	}
    }
}

// Content of called function
void
IQuantize (int *matrix, unsigned int *qmatrix)
{
  int *mptr;

  for (mptr = matrix; mptr < matrix + DCTSIZE2; mptr++)
    {
      *mptr = *mptr * (*qmatrix);
      qmatrix++;
    }
}

// Content of called function
void
IZigzagMatrix (int *imatrix, int *omatrix)
{
  int i;

  for (i = 0; i < DCTSIZE2; i++)
    
    {
      
*(omatrix++) = imatrix[zigzag_index[i]];
    
}
}

// Content of called function
void
PostshiftIDctMatrix (int *matrix, int shift)
{
  int *mptr;
  for (mptr = matrix; mptr < matrix + DCTSIZE2; mptr++)
    {
      *mptr += shift;
    }
}

// Content of called function
void
YuvToRgb (int p, int *y_buf, int *u_buf, int *v_buf)
{
  int r, g, b;
  int y, u, v;
  int i;

  for (i = 0; i < DCTSIZE2; i++)
    {
      y = y_buf[i];
      u = u_buf[i] - 128;
      v = v_buf[i] - 128;

      r = (y * 256 + v * 359 + 128) >> 8;
      g = (y * 256 - u * 88 - v * 182 + 128) >> 8;
      b = (y * 256 + u * 454 + 128) >> 8;

      if (r < 0)
	r = 0;
      else if (r > 255)
	r = 255;

      if (g < 0)
	g = 0;
      else if (g > 255)
	g = 255;

      if (b < 0)
	b = 0;
      else if (b > 255)
	b = 255;

      rgb_buf[p][0][i] = r;
      rgb_buf[p][1][i] = g;
      rgb_buf[p][2][i] = b;

    }
}

// Content of called function
void
Write4Blocks (int *store1, int *store2, int *store3, int *store4,
	      int *p_out_vpos, int *p_out_hpos, unsigned char *p_out_buf)
{
  int voffs, hoffs;

  /*
   * OX
   * XX
   */
  voffs = *p_out_vpos * DCTSIZE;
  hoffs = *p_out_hpos * DCTSIZE;
  WriteOneBlock (store1, p_out_buf,
		 p_jinfo_image_width, p_jinfo_image_height, voffs, hoffs);

  /*
   * XO
   * XX
   */
  hoffs += DCTSIZE;
  WriteOneBlock (store2, p_out_buf,
		 p_jinfo_image_width, p_jinfo_image_height, voffs, hoffs);

  /*
   * XX
   * OX
   */
  voffs += DCTSIZE;
  hoffs -= DCTSIZE;
  WriteOneBlock (store3, p_out_buf,
		 p_jinfo_image_width, p_jinfo_image_height, voffs, hoffs);


  /*
   * XX
   * XO
   */
  hoffs += DCTSIZE;
  WriteOneBlock (store4,
		 p_out_buf, p_jinfo_image_width, p_jinfo_image_height,
		 voffs, hoffs);

  /*
   * Add positions
   */
  *p_out_hpos = *p_out_hpos + 2;
  *p_out_vpos = *p_out_vpos + 2;


  if (*p_out_hpos < p_jinfo_MCUWidth)
    {
      *p_out_vpos = *p_out_vpos - 2;
    }
  else
    {
      *p_out_hpos = 0;		/* If at end of image (width) */
    }
}

// Content of called function
void
WriteOneBlock (int *store, unsigned char *out_buf, int width, int height,
	       int voffs, int hoffs)
{
  int i, e;


  /* Find vertical buffer offs. */
  for (i = voffs; i < voffs + DCTSIZE; i++)
    {
      if (i < height)
	{
	  int diff;
	  diff = width * i;
	  for (e = hoffs; e < hoffs + DCTSIZE; e++)
	    {
	      if (e < width)
		{
		  out_buf[diff + e] = (unsigned char) (*(store++));
		}
	      else
		{
		  break;
		}
	    }
	}
      else
	{
	  break;
	}
    }


}

// Content of called function
void
WriteBlock (int *store, int *p_out_vpos, int *p_out_hpos,
	    unsigned char *p_out_buf)
{
  int voffs, hoffs;

  /*
   * Get vertical offsets
   */
  voffs = *p_out_vpos * DCTSIZE;
  hoffs = *p_out_hpos * DCTSIZE;

  /*
   * Write block
   */
  WriteOneBlock (store,
		 p_out_buf,
		 p_jinfo_image_width, p_jinfo_image_height, voffs, hoffs);

  /*
   *  Add positions
   */
	(*p_out_hpos)++;
	(*p_out_vpos)++;

  if (*p_out_hpos < p_jinfo_MCUWidth)
    {
    (*p_out_vpos)--;
    }
  else
    {
      *p_out_hpos = 0;		/* If at end of image (width) */
    }
}