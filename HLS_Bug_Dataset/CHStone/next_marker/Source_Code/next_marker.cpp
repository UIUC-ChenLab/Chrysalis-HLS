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
int
read_byte (void)
{
  return *ReadBuf++;
}