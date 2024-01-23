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
int
read_byte (void)
{
  return *ReadBuf++;
}