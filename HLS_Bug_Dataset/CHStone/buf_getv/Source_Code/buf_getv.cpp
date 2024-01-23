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