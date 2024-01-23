void
DecodeHuffMCU (int *out_buf, int num_cmp)
{
  int s, diff, tbl_no, *mptr, k, n, r;

  /*
   * Decode DC
   */
  tbl_no = p_jinfo_comps_info_dc_tbl_no[num_cmp];
  s = DecodeHuffman (&p_jinfo_dc_xhuff_tbl_huffval[tbl_no][0],
		     p_jinfo_dc_dhuff_tbl_ml[tbl_no],
		     &p_jinfo_dc_dhuff_tbl_maxcode[tbl_no][0],
		     &p_jinfo_dc_dhuff_tbl_mincode[tbl_no][0],
		     &p_jinfo_dc_dhuff_tbl_valptr[tbl_no][0]);

  if (s)
    {
      diff = buf_getv (s);
      s--;
      if ((diff & bit_set_mask[s]) == 0)
	{
	  diff |= extend_mask[s];
	  diff++;
	}

      diff += *out_buf;		/* Change the last DC */
      *out_buf = diff;
    }

  /*
   * Decode AC
   */
  /* Set all values to zero */
  for (mptr = out_buf + 1; mptr < out_buf + DCTSIZE2; mptr++)
    {
      *mptr = 0;
    }

  for (k = 1; k < DCTSIZE2;)
    {				/* JPEG Mistake */
      r = DecodeHuffman (&p_jinfo_ac_xhuff_tbl_huffval[tbl_no][0],
			 p_jinfo_ac_dhuff_tbl_ml[tbl_no],
			 &p_jinfo_ac_dhuff_tbl_maxcode[tbl_no][0],
			 &p_jinfo_ac_dhuff_tbl_mincode[tbl_no][0],
			 &p_jinfo_ac_dhuff_tbl_valptr[tbl_no][0]);

      s = r & 0xf;		/* Find significant bits */
      n = (r >> 4) & 0xf;	/* n = run-length */

      if (s)
	{
	  if ((k += n) >= DCTSIZE2)	/* JPEG Mistake */
	    break;
	  out_buf[k] = buf_getv (s);	/* Get s bits */
	  s--;			/* Align s */
	  if ((out_buf[k] & bit_set_mask[s]) == 0)
	    {			/* Also (1 << s) */
	      out_buf[k] |= extend_mask[s];	/* Also  (-1 << s) + 1 */
	      out_buf[k]++;	/* Increment 2's c */
	    }
	  k++;			/* Goto next element */
	}
      else if (n == 15)		/* Zero run length code extnd */
	k += 16;
      else
	{
	  break;
	}
    }
}

// Content of called function
int
DecodeHuffman (int *Xhuff_huffval, int Dhuff_ml,
	       int *Dhuff_maxcode, int *Dhuff_mincode, int *Dhuff_valptr)
{
  int code, l, p;

  code = buf_getb ();
  for (l = 1; code > Dhuff_maxcode[l]; l++)
    {
      code = (code << 1) + buf_getb ();
    }

  if (code < Dhuff_maxcode[Dhuff_ml])
    {
      p = Dhuff_valptr[l] + code - Dhuff_mincode[l];
      return (Xhuff_huffval[p]);
    }
  else
    {
      main_result++;
      printf ("Huffman read error\n");
      EXIT;
    }

}

// Content of called function
int
buf_getb ()
{
  if (read_position < 0)
    {
      current_read_byte = pgetc ();
      read_position = 7;
    }

  if (current_read_byte & bit_set_mask[read_position--])
    {
      return (1);
    }
  return (0);
}

// Content of called function
static int
pgetc ()
{
  int temp;

  if ((temp = *CurHuffReadBuf++) == MARKER_MARKER)
    {				/* If MARKER then */
      if ((temp = *CurHuffReadBuf++))
	{			/* if next is not 0xff, then marker */
	  printf ("Unanticipated marker detected.\n");
	}
      else
	{
	  return (MARKER_MARKER);
	}
    }
  return (temp);
}

// Content of called function
int
buf_getv (int n)
{
  int p, rv;

  n--;
  p = n - read_position;
  while (p > 0)
    {
      if (read_position > 23)
	{			/* If byte buffer contains almost entire word */
	  rv = (current_read_byte << p);	/* Manipulate buffer */
	  current_read_byte = pgetc ();	/* Change read bytes */

	  rv |= (current_read_byte >> (8 - p));
	  read_position = 7 - p;
	  return (rv & lmask[n]);
	  /* Can return pending residual val */
	}
      current_read_byte = (current_read_byte << 8) | pgetc ();
      read_position += 8;	/* else shift in new information */
      p -= 8;
    }

  if (!p)
    {				/* If position is zero */
      read_position = -1;
      /* Can return current byte */
      return (current_read_byte & lmask[n]);
    }

  p = -p;
  /* Else reverse position and shift */
  read_position = p - 1;
  return ((current_read_byte >> p) & lmask[n]);
}