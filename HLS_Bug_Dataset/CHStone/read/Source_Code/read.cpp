int
read (unsigned char *s1, const unsigned char *s2, int n)
{
  unsigned char *p1;
  const unsigned char *p2;
  int n_tmp;
  
p1 = s1;
  p2 = s2;
  n_tmp = n;
  
while (n_tmp-- > 0)
    {
      *p1 = *p2;
      
p1++;
      
p2++;
    
}
  
return n;
}

// Content of called function
while (n_tmp-- > 0)
    {
      *p1 = *p2;
      
p1++;
      
p2++;
    
}