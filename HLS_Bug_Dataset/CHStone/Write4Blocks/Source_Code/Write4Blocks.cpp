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