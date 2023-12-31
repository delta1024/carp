#include <string.h>
#include <stdlib.h>
#define CARP_IMPL
#include "./src/carp.h"
#undef CARP_IMPL


result_t compile_run(Compile *c) {
    printf("name: %s, output: %s, \nsource_file: %s\n", c->name, c->output_file, c->source_file);
  return CARP_RESULT_OK;
}

#define try(exp) \
  if ((result = (exp)) != CARP_RESULT_OK) \
    return result

result_t build(int argc, char *argv[]) {
  IMPL_REBUILD(argv);
  result_t result = CARP_RESULT_OK;
  Compile c;
  try(compile_init(&c, "hello", "src/main.c", NULL, 0, false));
  try(compile_run(&c));

 return result;
}
