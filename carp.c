#define IMPL_CARP
#include "./src/carp.h"
#undef IMPL_CARP

int main(int argc, char *argv[]) {
  IMPL_REBUILD(argv, NULL);
  Compile c, t;
  if (!compile_init(&c, "hello", "src/main.c", NULL, 0, COMPILE_MODE_BINARY) ||
      !compile_init(&t, "temp", "src/temp.c", NULL, 0, COMPILE_MODE_OBJECT) ||
      !headers_append(&t.headers, "src/temp.h") || !deps_append(&c.deps, &t) ||
      !compile_run(&c))
    return 1;
}
