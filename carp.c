#define IMPL_CARP
#include "./src/carp.h"
#undef IMPL_CARP

int main(int argc, char *argv[]) {
  IMPL_REBUILD(argv, NULL);
  Compile c, temp;
  
  compile_init(&c, "hi", "src/main.c", COMPILE_MODE_BINARY);
  sys_libs_append(&c.sys_libs, "raylib");
  include_paths_append(&c.include_paths, "src");
  sys_lib_path_append(&c.sys_lib_paths, "/usr/lib");

  compile_init(&temp, "temp", "src/temp.c", COMPILE_MODE_OBJECT);
  headers_append(&temp.headers, "src/temp.h");

  deps_append(&c.deps, &temp);
  compile_run(&c);
}
