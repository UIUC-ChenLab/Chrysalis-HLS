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