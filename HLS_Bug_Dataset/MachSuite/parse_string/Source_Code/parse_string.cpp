int parse_string(char *s, char *arr, int n) {
  int k;
  assert(s!=NULL && "Invalid input string");

  if( n<0 ) { // terminated string
    k = 0;
    while( s[k]!=(char)0 && s[k+1]!=(char)0 && s[k+2]!=(char)0
        && !(s[k]=='\n'  && s[k+1]=='%'     && s[k+2]=='%') ) {
      k++;
    }
  } else { // fixed-length string
    k = n;
  }

  memcpy( arr, s, k );
  if( n<0 )
    arr[k] = 0;

  return 0;
}