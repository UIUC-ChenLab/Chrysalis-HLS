data_t sub(data_t ptr[10], data_t size, data_t **flagPtr) {
  data_t x, i;

  x = 0;
  // Sum x if AND of local index and double-pointer index is true
  for (i = 0; i < size; ++i)
    if (**flagPtr & i)
      x += *(ptr + i);
  return x;
}