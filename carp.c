#include <argp.h>
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

static struct argp_option options[] = {
    {"clean", 'c', 0, 0, "Clean build directory"},
    {"all", 'a', 0, 0, "Cleans all build artifacts; to be combiled with -c."},
    {"rebuild", 'r', 0, 0, "rebuild entire project from scratch"},
    {0},
};

struct arguments {
  bool clean, all, rebuild;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
  case 'c':
    arguments->clean = true;
    break;
  case 'a':
    if (!arguments->clean) {
      argp_state_help(state, stderr, ARGP_HELP_LONG);
      return ARGP_KEY_ERROR;
    }
    arguments->all = true;
    break;
  case 'r':
    arguments->rebuild = true;
    break;
  case ARGP_KEY_ARG:
    if (state->arg_num != 0)
      argp_usage(state);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, 0, 0};

result_t config_build();
result_t build(int argc, char *argv[]) {

  struct arguments arguments = {false, false, false};

  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  char *args_str = NULL;
  if (arguments.rebuild) {
    args_str = "-r";
  }

  if (arguments.clean) {
    if (arguments.all) {
      args_str = "-ca";
    } else {
      args_str = "-c";
    }
  }

  IMPL_REBUILD(argv, args_str);
  if (arguments.rebuild) {
    if (system("./carp --clean") != 0) {
      return CARP_ERR_COMPILE;
    }
    system("./carp");
    return CARP_RESULT_OK;
  }

  if (arguments.clean == false && arguments.all) {
    return CARP_ERR_BAD_ARGS;
  }

  if (arguments.clean) {
    DIR *b = opendir("build");
    if (b != NULL) {
      closedir(b);
      carp_log(CARP_LOG_INFO, "rm -r build");
      system("rm -r build");
    }
    if (arguments.all) {
      carp_log(CARP_LOG_INFO, "rm carp");
      system("rm carp");
    }
    return CARP_RESULT_OK;
  }

  return config_build();
}
result_t config_build() {
  result_t result = CARP_RESULT_OK;

  Compile c, t;
  try(compile_init(&c, "hello", "src/main.c", NULL, 0, false));

  init_obj(&t, "temp", "src/temp.c");
  try(headers_append(&t.headers, "src/temp.h"));

  try(deps_append(&c.deps, &t));
  try(compile_run(&c));

  return result;
}
