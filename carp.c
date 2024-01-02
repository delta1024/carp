#include <stdlib.h>
#define CARP_HEADER_PATH "./src/carp.h"
#define IMPL_CARP
#include "./src/carp.h"
#undef IMPL_CARP

struct object_file {
  char *name, *src, *header;
  Compile c;
};

static struct object_file objs[] = {
    {"temp", "src/temp.c", "src/temp.h", {}},
};


bool init_obj(struct object_file *file) {
  if (!compile_init(&file->c, file->name, file->src, COMPILE_MODE_OBJECT))
    return false;
  if (!headers_append(&file->c, file->header))
    return false;
  return true;
}

void compile_program();

int main(int argc, char *argv[]) {
  IMPL_REBUILD(argc, argv);

  if (argc >= 2) {
    if (strcmp("clean", argv[1]) == 0) {

      Cmd cmd = {0};
      cmd_append(&cmd, "rm", "-r", "build");
      cmd_run(&cmd);
      exit(EXIT_SUCCESS);
    }
  }

  compile_program();
}

void compile_program() {
  Compile c;

  compile_init(&c, "hi", "src/main.c", COMPILE_MODE_BINARY);

  int objs_num = sizeof(objs) / sizeof(struct object_file);
  for (int i = 0; i < objs_num; i++) {
    if (!init_obj(&objs[i])) {
      carp_perror("init_obj");
      exit(EXIT_FAILURE);
    }
    deps_append(&c.deps, &objs[i].c);
  }

  compile_run(&c);
}
