#define CARP_IMPL
#include "./src/carp.h"
#undef CARP_IMPL

result_t build(int argc, char *argv[]) {
  IMPL_REBUILD(argv);
  printf("Hello worl\n");
  return CARP_RESULT_OK;
}
