int write_string(int fd, char *arr, int n) {
  int status, written;
  assert(fd>1 && "Invalid file descriptor");
  if( n<0 ) { // NULL-terminated string
    n = strlen(arr);
  }
  written = 0;
  while(written<n) {
    status = write(fd, &arr[written], n-written);
    assert(status>=0 && "Write failed");
    written += status;
  }
  // Write terminating '\n'
  do {
    status = write(fd, "\n", 1);
    assert(status>=0 && "Write failed");
  } while(status==0);

  return 0;
}