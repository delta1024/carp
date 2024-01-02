#ifndef CARP_H_
#define CARP_H_

#include <dirent.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/************************************************************
 *                       carp.h                             *
 ************************************************************/

// typedef struct Builder Builder;
typedef enum CARP_LOG_LEVEL {
  CARP_LOG_FATAL = 0,
  CARP_LOG_ERR,
  CARP_LOG_INFO,
  CARP_LOG_CMD,
} CARP_LOG_LEVEL;

typedef enum CARP_RESULT {
  CARP_RESULT_OK = 0,
  CARP_ERR_NOMEM = -1,
  CARP_ERR_COMPILE = -2,
  CARP_ERR_BAD_ARGS = -3,
} result_t;

static int carp_errno = 0;

bool is_newer(char *to_check, char *older);
bool carp_directory_exists(char *path);
bool carp_file_exists(char *path);
void carp_log(CARP_LOG_LEVEL level, const char *fmt, ...);
void carp_perror(const char *fmt);

/************************************************************
 *                      sb.h                                *
 ************************************************************/

typedef struct {
  char *ptr;
  int cap;
} StringBuilder;

/**
 * @returns false if an error has occured.
 */
bool sb_append(StringBuilder *sb, char *arg);
/**
 * @returns false if an error has occured.
 */
bool sb_append_many(StringBuilder *sb, char **args, int args_len);

/************************************************************
 *                     cmd.h                                *
 ************************************************************/
typedef struct Cmd {
  char **items;
  int cap, count;
} Cmd;

bool cmd_append_arg(Cmd *cmd, char *arg);
bool cmd_append_args(Cmd *cmd, int arg_len, ...);
#define cmd_append(cmd, ...)                                                   \
  (cmd_append_args(cmd, (sizeof((char *[]){__VA_ARGS__}) / sizeof(char *)),    \
                   __VA_ARGS__))
bool cmd_run(Cmd *cmd);

/************************************************************
 *                    compile.h                             *
 ************************************************************/
typedef struct Compile Compile;

typedef struct Deps {
  Compile **ptr;
  int count;
  int cap;
} Deps;



typedef struct Args {
  char **ptr;
  int cap, count;
} Args;

typedef struct Args IncludePaths;
typedef struct Args SysLibs;
typedef struct Args SysLibPath;
typedef struct Args Headers;
typedef enum CompileMode {
  COMPILE_MODE_BINARY,
  COMPILE_MODE_OBJECT,
} CompileMode;

struct Compile {
  char *name, *output_file, *source_file;
  Deps deps;
  Args args;
  IncludePaths include_paths;
  SysLibPath sys_lib_paths;
  Headers headers;
  SysLibs sys_libs;
  CompileMode mode;
  int args_len;
};

bool compile_init(Compile *c, char *name, char *source_file, CompileMode mode);
bool compile_needs_rebuild(Compile *c);
bool compile_run(Compile *c);
bool deps_append(Deps *d, Compile *c);
bool args_append_arg(Args *args, char *path);
bool args_append_many(Args *args, int count, ...);
#define args_append(arg, ...)                                                  \
  (args_append_many((arg), (sizeof((char *[]){__VA_ARGS__}) / sizeof(char *)), \
                    __VA_ARGS__))
#define include_paths_append(inc, path) (args_append_arg((Args*)(inc), (path)))
#define sys_libs_append(inc, path) (args_append_arg((Args*)(inc), (path)))
#define sys_lib_path_append(inc, path) (args_append_arg((Args*)(inc), (path)))
#define headers_append(inc, path) (args_append_arg((Args*)(inc), (path)))

char *make_flag(char flag, char *name, char *value);

#ifdef IMPL_CARP
/************************************************************
 *                       carp.c                             *
 ************************************************************/

bool is_newer(char *to_check, char *older) {
  struct stat src_attr, prog_attr;
  if (stat(older, &prog_attr) == -1) {
    perror("could not get process stats");
    exit(EXIT_FAILURE);
  }
  if (stat(to_check, &src_attr) == -1) {
    perror("could not get src file stats");
    exit(EXIT_FAILURE);
  }
  if (prog_attr.st_mtim.tv_sec < src_attr.st_mtim.tv_sec) {
    return true;
  }
  return false;
}
bool carp_file_exists(char *path) { return access(path, F_OK) == 0; }
bool carp_directory_exists(char *path) {
  DIR *d;
  if ((d = opendir(path)) == NULL) {
    return false;
  } else {
    closedir(d);
    return true;
  }
}
void carp_log(CARP_LOG_LEVEL level, const char *fmt, ...) {
  char *log_message[] = {"[FATAL]", "[ERROR]", "[INFO]", "[CMD]"};
  fprintf(level > CARP_LOG_ERR ? stdout : stderr, "%s ", log_message[level]);
  va_list args;
  va_start(args, fmt);
  vfprintf(level > CARP_LOG_ERR ? stdout : stderr, fmt, args);
  va_end(args);
  putchar('\n');
}

void carp_perror(const char *fmt) {
  char *err_str;

  switch ((result_t)carp_errno) {
  case CARP_ERR_NOMEM:
    err_str = "Out of memory.";
    break;
  case CARP_ERR_COMPILE:
    err_str = "Could not build file.";
    break;
  case CARP_ERR_BAD_ARGS:
    err_str = "Bad input arguments";
    break;
  default:
    break;
  }
  carp_log(CARP_LOG_ERR, "%s: %s", fmt, err_str);
}

/************************************************************
 *                       sb.c                               *
 ************************************************************/

bool sb_append(StringBuilder *sb, char *arg) {
  int arg_len = strlen(arg);
  int str_len;
  if (sb->ptr != NULL) {
    str_len = strlen(sb->ptr);
  } else {
    str_len = 0;
  }
  sb->ptr = (char *)realloc(sb->ptr, arg_len + str_len + 3);
  if (sb->ptr == NULL) {
    carp_errno = CARP_ERR_NOMEM;
    return false;
  }
  if (str_len != 0) {
    sb->ptr[str_len] = ' ';
    strcpy(&sb->ptr[str_len + 1], arg);
  } else {
    strcpy(sb->ptr, arg);
  }

  return true;
}

bool sb_append_many(StringBuilder *sb, char **args, int args_len) {
  result_t result;
  for (int i = 0; i < args_len; i++) {
    if (!sb_append(sb, args[i])) {
      return false;
    }
  }
  return true;
}

/************************************************************
 *                     cmd.c                                *
 ************************************************************/
// NOLINTNEXTLINE(misc-definitions-in-headers)
bool cmd_append_arg(Cmd *cmd, char *arg) {
  if (cmd->cap < cmd->count + 1) {
    int old_cap = cmd->cap;
    cmd->cap = old_cap < 8 ? 8 : old_cap * 2;
    cmd->items = realloc(cmd->items, sizeof(char *) * cmd->cap);
    if (cmd->items == NULL) {
      carp_errno = CARP_ERR_NOMEM;
      return false;
    }
  }
  cmd->items[cmd->count++] = arg;
  return true;
}
// NOLINTNEXTLINE(misc-definitions-in-headers)
bool cmd_append_args(Cmd *cmd, int arg_num, ...) {
  va_list args;
  va_start(args, arg_num);
  for (int i = 0; i < arg_num; i++) {
    char *arg = va_arg(args, char *);
    if (!cmd_append_arg(cmd, arg)) {
      carp_perror("cmd_append_arg");
      return false;
    }
  }
  va_end(args);
  return true;
}
// NOLINTNEXTLINE(misc-definitions-in-headers)
bool cmd_run(Cmd *cmd) {
  StringBuilder sb = {0};

  if (cmd->items == NULL)
    return true;

  if (!sb_append_many(&sb, cmd->items, cmd->count)) {
    carp_perror("cmd_run");
    return false;
  }
  carp_log(CARP_LOG_CMD, sb.ptr);
  if (system(sb.ptr) < 0) {
    perror("cmd_run");
    return false;
  }
  free(sb.ptr);
  return true;
}
/************************************************************
 *                 compile.c                                *
 ************************************************************/

// NOLINTNEXTLINE(misc-definitions-in-headers)

bool deps_append(Deps *d, Compile *c) {
  if (d->cap < d->count + 1) {
    int old_cap = d->cap;
    d->cap = old_cap < 8 ? 8 : old_cap * 2;
    d->ptr = (Compile **)realloc(d->ptr, sizeof(Compile *) * d->cap);
    if (d->ptr == NULL) {
      carp_errno = CARP_ERR_NOMEM;
      return false;
    }
  }
  d->ptr[d->count++] = c;
  return true;
}
bool args_append_arg(Args *args, char *path) {
  if (args->count + 1 > args->cap) {
    args->cap = args->cap < 8 ? 8 : args->cap * 2;
    args->ptr = realloc(args->ptr, sizeof(char *) * args->cap);
    if (args->ptr == NULL) {
      carp_errno = CARP_ERR_NOMEM;
      return false;
    }
  }
  args->ptr[args->count++] = path;
  return true;
}
bool args_append_many(Args *args, int count, ...) {
  va_list func_args;
  va_start(func_args, count);
  for (int i = 0; i < count; i++) {
    char *ptr = va_arg(func_args, char *);
    if (!args_append_arg(args, ptr)) {
      carp_perror("args_append_many");
      return false;
    }
  }
  return true;
}

char *make_flag(char flag, char *name, char *value) {
  char *fmt_str;
  int str_len;
  if (value == NULL) {
    fmt_str = "-%c%s";
    str_len = strlen(name) + 3;
  } else {
    fmt_str = "-%c%s=%s";
    str_len = strlen(name) + strlen(value) + 4;
  }
  char *buff;
  if ((buff = malloc(str_len)) == NULL) {
    carp_errno = CARP_ERR_NOMEM;
    return NULL;
  }

  sprintf(buff, fmt_str, flag, name, value);
  return buff;
}

// NOLINTNEXTLINE(misc-definitions-in-headers)
bool compile_init(Compile *c, char *name, char *source_file, CompileMode mode) {
  bool result;
  *c = (Compile){0};

  c->name = malloc(strlen(name) + 1);
  if (c->name == NULL) {
    carp_errno = CARP_ERR_NOMEM;
    result = false;
    goto no_dealloc;
  }
  strcpy(c->name, name);

  c->source_file = malloc(strlen(source_file) + 1);
  if (c->source_file == NULL) {
    result = false;
    carp_errno = CARP_ERR_NOMEM;
    goto free_name;
  }
  strcpy(c->source_file, source_file);

  int pad_len;
  char *fmt_str;
  switch (mode) {
  case COMPILE_MODE_OBJECT: {
    pad_len = 3;
    fmt_str = "build/%s.o";
  } break;
  case COMPILE_MODE_BINARY: {
    pad_len = 2;
    fmt_str = "build/%s";
  } break;
  default:
    break;
  }

  c->mode = mode;
  c->output_file = malloc(strlen(name) + pad_len + strlen("build/"));
  if (c->output_file == NULL) {
    result = false;
    carp_errno = CARP_ERR_NOMEM;
    goto free_dest;
  }
  sprintf(c->output_file, fmt_str, name);

  result = true;
  goto no_dealloc;

free_dest:
  free(c->output_file);
  c->output_file = NULL;
free_name:
  free(c->name);
  c->name = NULL;

no_dealloc:
  return result;
}

bool compile_needs_rebuild(Compile *c) {
  if (!(carp_file_exists(c->output_file) &&
        is_newer(c->output_file, c->source_file)))
    return true;

  for (int i = 0; i < c->headers.count; i++) {
    if (!is_newer(c->output_file, c->headers.ptr[i]))
      return true;
  }

  for (int i = 0; i < c->deps.count; i++) {
    if (compile_needs_rebuild(c->deps.ptr[i]))
      return true;
  }

  return false;
}

bool compile_run(Compile *c) {

  if (!carp_directory_exists("build"))
    system("mkdir build");

  if (!compile_needs_rebuild(c)) {
    return true;
  }

  for (int i = 0; i < c->deps.count; i++) {
    if (!compile_run(c->deps.ptr[i])) {
      return false;
    }
  }

  Cmd cmd = {0};
  if (!cmd_append(&cmd, "cc", "-o", c->output_file))
    return false;

  switch (c->mode) {
  case COMPILE_MODE_OBJECT: {
    if (!cmd_append(&cmd, "-c"))
      return false;
  } break;
  case COMPILE_MODE_BINARY:
    break;
  }

  for (int i = 0; i < c->sys_lib_paths.count; i++) {
    char *lib;
    if ((lib = make_flag('L', c->sys_lib_paths.ptr[i], NULL)) == NULL) {
      carp_perror("compile");
      return false;
    }
    if (!cmd_append(&cmd, lib))
      return false;
  }

  for (int i = 0; i < c->sys_libs.count; i++) {
    char *lib;
    if ((lib = make_flag('l', c->sys_libs.ptr[i], NULL)) == NULL) {
      carp_perror("compile");
      return false;
    }
    if (!cmd_append(&cmd, lib))
      return false;
  }

  for (int i = 0; i < c->include_paths.count; i++) {
    char *flag;
    if ((flag = make_flag('I', c->include_paths.ptr[i], NULL)) == NULL) {
      carp_perror("compile");
      return false;
    }
    if (!cmd_append_arg(&cmd, flag))
      return false;
  }

  if (c->args.count > 0) {
    for (int i = 0; i < c->args.count; i++) {
      if (!cmd_append_arg(&cmd, c->args.ptr[i]))
        return false;
    }
  }

  if (!cmd_append(&cmd, c->source_file))
    return false;

  for (int i = 0; i < c->deps.count; i++) {
    if (!cmd_append_arg(&cmd, c->deps.ptr[i]->output_file)) {
      return false;
    }
  }

  if (!cmd_run(&cmd)) {
    carp_errno = CARP_ERR_COMPILE;
    return false;
  }

  free(cmd.items);
  return true;
}





#define IMPL_REBUILD(argv, args_str)                                           \
  do {                                                                         \
    if (is_newer(__FILE__, argv[0]) || is_newer("./src/carp.h", argv[0])) {    \
      carp_log(CARP_LOG_INFO, "rebuilding carp");                              \
      carp_log(CARP_LOG_CMD, "cc -o carp "__FILE__);                           \
      system("cc -o carp "__FILE__);                                           \
      if (args_str == NULL) {                                                  \
        system("./carp");                                                      \
      } else {                                                                 \
        StringBuilder sb = {0};                                                \
        char *args[] = {                                                       \
            "./carp",                                                          \
            args_str,                                                          \
        };                                                                     \
        if (!sb_append_many(&sb, args, sizeof(args) / sizeof(char *))) {       \
          return CARP_ERR_NOMEM;                                               \
        }                                                                      \
        system(sb.ptr);                                                        \
      }                                                                        \
      return CARP_RESULT_OK;                                                   \
    }                                                                          \
  } while (false)
#endif

#endif // CARP_H_
