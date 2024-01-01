#define IMPL_CARP
#include "./src/carp.h"
#undef IMPL_CARP

#define add_dependency(c, d, name, source,header) \
  !compile_init(&(d), (name), (source), NULL, 0, COMPILE_MODE_OBJECT) || \
  !headers_append(&(d).headers, (header)) || !deps_append(&(c).deps, &(d))
int main(int argc, char *argv[]) {
  IMPL_REBUILD(argv, NULL);
  Compile c, t;
  if (!compile_init(&c, "hello", "src/main.c", NULL, 0, COMPILE_MODE_BINARY) ||
      add_dependency(c, t, "temp", "src/temp.c", "src/temp.h")       ||
      
      !compile_run(&c))
    return 1;
}
