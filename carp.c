#include <dirent.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#define CARP_IMPL
#include "./src/carp.h"
#undef CARP_IMPL

#define try(exp)                                                               \
  if ((result = (exp)) != CARP_RESULT_OK)                                      \
  return result

#define init_obj(c, name, path)                                                \
  try(compile_init((c), (name), (path), NULL, 0, true))

enum CARP_ARGS_RESULT {
  CARP_ARGS_NO_RUN,
  CARP_ARGS_RAN,
};

enum CARP_ARGS_RESULT parse_args(int argc, char *argv[]) {
  if  (argc >= 2 && strcmp("clean", argv[1]) == 0) {
      DIR *ck = opendir("./build");
      if (ck != NULL) {
	closedir(ck);
	carp_log(CARP_LOG_INFO, "rm -r ./build");
	system("rm -r ./build");      
      }

    if (argc == 3 && strcmp("all", argv[2]) == 0) {
      carp_log(CARP_LOG_INFO, "rm ./carp");
      system("rm ./carp");
    }

    if (argc == 3 && strcmp("rebuild", argv[2]) == 0) {
      carp_log(CARP_LOG_INFO, "./carp");
      system("./carp");
 
    }
    
    return CARP_ARGS_RAN;
  }
  return CARP_ARGS_NO_RUN;

}

result_t build(int argc, char *argv[]) {
  if (parse_args(argc, argv) == CARP_ARGS_RAN) {
    return CARP_RESULT_OK;
  }

  IMPL_REBUILD(argv);
  result_t result = CARP_RESULT_OK;

  Compile c, t;
  try(compile_init(&c, "hello", "src/main.c", NULL, 0, false));

  init_obj(&t, "temp", "src/temp.c");
  try(headers_append(&t.headers, "src/temp.h"));
  
  try(deps_append(&c.deps, &t));
  try(compile_run(&c));

  return result;
}




