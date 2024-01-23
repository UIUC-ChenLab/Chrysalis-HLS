#define    M_EOI   0xd9		/* End of Image */

#define    M_DQT   0xdb		/* Quantization Table */

#define    M_DHT   0xc4		/* Huffman Table */

#define    M_SOS   0xda		/* Start of Scan ( Head of Compressed Data ) */

#define    M_SOF0  0xc0		/* Baseline DCT ( Huffman ) */

#define    M_SOI   0xd8		/* Start of Image */

void
read_markers (unsigned char *buf)
{
  int unread_marker;
  int sow_SOI;

  ReadBuf = buf;

  sow_SOI = 0;

  unread_marker = 0;

  /* Read the head of the marker */
  for (;;)
    {
      if (!sow_SOI)
	{
	  unread_marker = first_marker ();
	}
      else
	{
	  unread_marker = next_marker ();
	}

      printf ("\nmarker = 0x%x\n", unread_marker);

      if (unread_marker != out_unread_marker[i_marker++])
	{
		main_result++;
	}


      switch (unread_marker)
	{
	case M_SOI:		/* Start of Image */
	  sow_SOI = 1;
	  break;

	case M_SOF0:		/* Baseline DCT ( Huffman ) */
	  get_sof ();
	  break;

	case M_SOS:		/* Start of Scan ( Head of Compressed Data ) */
	  get_sos ();
	  return;

	case M_DHT:
	  get_dht ();
	  break;

	case M_DQT:
	  get_dqt ();
	  break;

	case M_EOI:
	  return;
	}
    }
}

// Content of called function
void
get_sos ()
{
  int length, num_comp;
  int i, c, cc, ci, j;
  char *p_comp_info_id;
  char *p_comp_info_dc_tbl_no;
  char *p_comp_info_ac_tbl_no;

  length = read_word ();
  num_comp = read_byte ();

  printf (" length = %d\n", length);
  printf (" num_comp = %d\n", num_comp);

  if (length != out_length_get_sos)
    {
        main_result++;
    }
  if (num_comp != out_num_comp_get_sos)
    {
        main_result++;
    }

  /* Decode each component */
  for (i = 0; i < num_comp; i++)
    {
      cc = read_byte ();
      c = read_byte ();

      for (ci = 0; ci < p_jinfo_num_components; ci++)
	{
	  p_comp_info_id = &p_jinfo_comps_info_id[ci];
	  p_comp_info_dc_tbl_no = &p_jinfo_comps_info_dc_tbl_no[ci];
	  p_comp_info_ac_tbl_no = &p_jinfo_comps_info_ac_tbl_no[ci];

	  if (cc == *p_comp_info_id)
	    goto id_found;
	}
      main_result++;
      printf ("Bad Component ID!\n");
      EXIT;

    id_found:
      *p_comp_info_dc_tbl_no = (c >> 4) & 15;
      *p_comp_info_ac_tbl_no = (c) & 15;

      printf (" comp_id       = %d\n", cc);
      printf (" dc_tbl_no     = %d\n", *p_comp_info_dc_tbl_no);
      printf (" ac_tbl_no     = %d\n", *p_comp_info_ac_tbl_no);

      if (cc != out_comp_id_get_sos[i_get_sos])
	{
		main_result++;
	}
      if (*p_comp_info_dc_tbl_no != out_dc_tbl_no_get_sos[i_get_sos])
	{
		main_result++;
	}
      if (*p_comp_info_ac_tbl_no != out_ac_tbl_no_get_sos[i_get_sos])
	{
		main_result++;
	}
      i_get_sos++;

    }

  /* Pass parameters; Ss, Se, Ah and Al for progressive JPEG */
  j = 3;
  while (j--)
    {
      c = read_byte ();
    }

  /*
   * Define the Buffer at this point as the head of data
   */
  p_jinfo_jpeg_data = ReadBuf;

}

// Content of called function
short
read_word (void)
{
  short c;

  c = *ReadBuf++ << 8;
  c |= *ReadBuf++;

  return c;
}

// Content of called function
int
read_byte (void)
{
  return *ReadBuf++;
}

// Content of called function
int
next_marker (void)
{
  int c;

  for (;;)
    {
      c = read_byte ();

      while (c != 0xff)
	c = read_byte ();

      do
	{
	  c = read_byte ();
	}
      while (c == 0xff);
      if (c != 0)
	break;
    }
  return c;
}

// Content of called function
#define    M_SOI   0xd8		/* Start of Image */

int
first_marker (void)
{
  int c1, c2;
  c1 = read_byte ();
  c2 = read_byte ();

  if (c1 != 0xFF || c2 != M_SOI)
    {
      main_result++;
      printf ("Not Jpeg File!\n");
      EXIT;
    }

  return c2;
}

// Content of called function
void
get_sof ()
{
  int ci, c;
  int length;
  char *p_comp_info_index;
  char *p_comp_info_id;
  char *p_comp_info_h_samp_factor;
  char *p_comp_info_v_samp_factor;
  char *p_comp_info_quant_tbl_no;

  length = read_word ();
  p_jinfo_data_precision = read_byte ();
  p_jinfo_image_height = read_word ();
  p_jinfo_image_width = read_word ();
  p_jinfo_num_components = read_byte ();

  printf ("length         = %d\n", length);
  printf ("data_precision = %d\n", p_jinfo_data_precision);
  printf ("image_height   = %d\n", p_jinfo_image_height);
  printf ("image_width    = %d\n", p_jinfo_image_width);
  printf ("num_components = %d\n", p_jinfo_num_components);

  if (length != out_length_get_sof)
    {
        main_result++;
    }
  if (p_jinfo_data_precision != out_data_precision_get_sof)
    {
        main_result++;
    }
  if (p_jinfo_image_height != out_p_jinfo_image_height_get_sof)
    {
        main_result++;
    }
  if (p_jinfo_image_width != out_p_jinfo_image_width_get_sof)
    {
        main_result++;
    }
  if (p_jinfo_num_components != out_p_jinfo_num_components_get_sof)
    {
        main_result++;
    }

  length -= 8;

  /* Omit error check */

  /* Check components */
  for (ci = 0; ci < p_jinfo_num_components; ci++)
    {
      p_comp_info_index = &p_jinfo_comps_info_index[ci];
      p_comp_info_id = &p_jinfo_comps_info_id[ci];
      p_comp_info_h_samp_factor = &p_jinfo_comps_info_h_samp_factor[ci];
      p_comp_info_v_samp_factor = &p_jinfo_comps_info_v_samp_factor[ci];
      p_comp_info_quant_tbl_no = &p_jinfo_comps_info_quant_tbl_no[ci];

      *p_comp_info_index = ci;
      *p_comp_info_id = read_byte ();
      c = read_byte ();
      *p_comp_info_h_samp_factor = (c >> 4) & 15;
      *p_comp_info_v_samp_factor = (c) & 15;
      *p_comp_info_quant_tbl_no = read_byte ();

      printf (" index         = %d\n", *p_comp_info_index);
      printf (" id            = %d\n", *p_comp_info_id);
      printf (" h_samp_factor = %d\n", *p_comp_info_h_samp_factor);
      printf (" v_samp_factor = %d\n", *p_comp_info_v_samp_factor);
      printf (" quant_tbl_no  = %d\n\n", *p_comp_info_quant_tbl_no);

      if (*p_comp_info_index != out_index_get_sof[ci])
	{
		main_result++;
	}
      if (*p_comp_info_id != out_id_get_sof[ci])
	{
		main_result++;
	}
      if (*p_comp_info_h_samp_factor != out_h_samp_factor_get_sof[ci])
	{
		main_result++;
	}
      if (*p_comp_info_v_samp_factor != out_v_samp_factor_get_sof[ci])
	{
		main_result++;
	}
      if (*p_comp_info_quant_tbl_no != out_quant_tbl_no_get_sof[ci])
	{
		main_result++;
	}

    }

  /*
   *  Determine Sampling Factor
   *  Only 1_1_1 and 4_1_1 are supported
   */
  if (p_jinfo_comps_info_h_samp_factor[0] == 2)
    {
      p_jinfo_smp_fact = SF4_1_1;
      printf ("\nSampling Factor is 4:1:1\n");
    }
  else
    {
      p_jinfo_smp_fact = SF1_1_1;
      printf ("\nSampling Factor is 1:1:1\n");
    }
}

// Content of called function
void
get_dht ()
{
  int length;
  int index, i, count;
  int *p_xhtbl_bits;
  int *p_xhtbl_huffval;

  length = read_word ();
  length -= 2;

  printf (" length = %d\n", length);

  if (length != out_length_get_dht[i_get_dht])
    {
        main_result++;
    }

  while (length > 16)
    {
      index = read_byte ();

      printf (" index = 0x%x\n", index);

      if (index != out_index_get_dht[i_get_dht])
	{
        main_result++;
}

      if (index & 0x10)
	{
	  /* AC */
	  index -= 0x10;
	  p_xhtbl_bits = p_jinfo_ac_xhuff_tbl_bits[index];
	  p_xhtbl_huffval = p_jinfo_ac_xhuff_tbl_huffval[index];
	}
      else
	{
	  /* DC */
	  p_xhtbl_bits = p_jinfo_dc_xhuff_tbl_bits[index];
	  p_xhtbl_huffval = p_jinfo_dc_xhuff_tbl_huffval[index];
	}

      count = 0;

      for (i = 1; i <= 16; i++)
	{
	  p_xhtbl_bits[i] = read_byte ();
	  count += p_xhtbl_bits[i];
	}

      printf (" count = %d\n", count);

      if (count != out_count_get_dht[i_get_dht])
	{
        main_result++;
    }
      i_get_dht++;

      length -= 1 + 16;

      for (i = 0; i < count; i++)
	{
	  p_xhtbl_huffval[i] = read_byte ();
	}

      length -= count;
    }
}

// Content of called function
void
get_dqt ()
{
  int length;
  int prec, num, i;
  unsigned int tmp;
  unsigned int *p_quant_tbl;

  length = read_word ();
  length -= 2;

  printf (" length = %d\n", length);

  if (length != out_length_get_dqt[i_get_dqt])
    {
        main_result++;
    }

  while (length > 0)
    {
      num = read_byte ();
      /* Precision 0:8bit, 1:16bit */
      prec = num >> 4;
      /* Table Number */
      num &= 0x0f;

      printf (" prec = %d\n", prec);
      printf (" num  = %d\n", num);

      if (prec != out_prec_get_dht[i_get_dqt])
	{
        main_result++;
    }
      if (num != out_num_get_dht[i_get_dqt])
	{
        main_result++;
    }
      i_get_dqt++;

      p_quant_tbl = &p_jinfo_quant_tbl_quantval[num][DCTSIZE2];
      for (i = 0; i < DCTSIZE2; i++)
	{
	  if (prec)
	    tmp = read_word ();
	  else
	    tmp = read_byte ();
	  p_quant_tbl[izigzag_index[i]] = (unsigned short) tmp;
	}

      length -= DCTSIZE2 + 1;
      if (prec)
	length -= DCTSIZE2;
    }
}