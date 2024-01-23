void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  FIXME( &(args->ctx), args->k, args->buf );
}