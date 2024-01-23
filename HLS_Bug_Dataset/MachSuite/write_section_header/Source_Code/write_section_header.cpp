int write_section_header(int fd) {
  assert(fd>1 && "Invalid file descriptor");
  fd_printf(fd, "%%%%\n"); // Just prints %%
  return 0;
}