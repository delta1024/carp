#ifndef CARP_H_
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
  CARP_LOG_COMMAND,
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
 * @returns true if an error has occured.
 */
bool sb_append(StringBuilder *sb, char *arg);
/**
 * @returns true if an error has occured.
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

typedef struct Headers {
  char **ptr;
  int cap, count;
} Headers;

typedef enum CompileMode {
  COMPILE_MODE_BINARY,
  COMPILE_MODE_OBJECT,
} CompileMode;

struct Compile {
  char *name, *output_file, *source_file, **args;
  Deps deps;
  Headers headers;
  CompileMode mode;
  int args_len;
};

bool compile_init(Compile *c, char *name, char *source_file, char *args[],
                  int arg_len, CompileMode mode);
bool compile_needs_rebuild(Compile *c);
bool compile_run(Compile *c);
bool deps_append(Deps *d, Compile *c);
bool headers_append(Headers *header, char *path);
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
  char *log_message[] = {"[FATAL]", "[ERROR]", "[INFO]", "[COMMAND]"};
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
  return true;
}

bool cmd_run(Cmd *cmd) {
  StringBuilder sb = {0};

  if (cmd->items == NULL)
    return true;

  if (!sb_append_many(&sb, cmd->items, cmd->count)) {
    carp_perror("cmd_run");
    return false;
  }
  carp_log(CARP_LOG_INFO, sb.ptr);
  if (system(sb.ptr) < 0) {
    perror("cmd_run");
    return false;
  }
  free(sb.ptr);
  return true;
}

/************************************************************
 *                    compile.c                             *
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
bool headers_append(Headers *header, char *arg) {
  if (header->cap < header->count + 1) {
    int old_cap = header->cap;
    header->cap = old_cap < 8 ? 8 : old_cap * 2;
    header->ptr = realloc(header->ptr, sizeof(char *) * header->cap);
    if (header->ptr == NULL) {
      carp_errno = CARP_ERR_NOMEM;
      return false;
    }
  }
  header->ptr[header->count++] = arg;
  return true;
}
// NOLINTNEXTLINE(misc-definitions-in-headers)
bool compile_init(Compile *c, char *name, char *source_file, char *args[],
                  int arg_len, CompileMode mode) {
  bool result;
  *c = (Compile){NULL, NULL, NULL, NULL, (Deps){0}, (Headers){0}, false, 0};

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

  switch (mode) {
  case COMPILE_MODE_OBJECT: {
    c->mode = COMPILE_MODE_OBJECT;
    c->output_file = malloc(strlen(name) + 3 + strlen("build/"));
    if (c->output_file == NULL) {
      result = false;
      carp_errno = CARP_ERR_NOMEM;
      goto free_dest;
    }
    sprintf(c->output_file, "build/%s.o", name);
  } break;
  case COMPILE_MODE_BINARY:{
      c->mode = COMPILE_MODE_BINARY;
    c->output_file = malloc(strlen(name) + 1 + strlen("build/"));
    if (c->output_file == NULL) {
      result = false;
      carp_errno = CARP_ERR_NOMEM;
      goto free_dest;
    }
    sprintf(c->output_file, "build/%s", name);
    } break;
  default:
    break;
  }
  if (args != NULL) {
    c->args = malloc(sizeof(char *) * arg_len);
    if (c->args == NULL) {
      result = false;
      carp_errno = CARP_ERR_NOMEM;
      goto free_dest;
    }
    for (int i = 0; i < arg_len; i++) {
      char *buf = malloc(strlen(args[i]) + 1);
      if (buf == NULL) {
        result = false;
        carp_errno = CARP_ERR_NOMEM;
        goto free_args;
      }
      strcpy(c->args[i], buf);
      c->args_len++;
    }
  }

  result = true;
  goto no_dealloc;

free_args:
  if (c->args != NULL) {
    for (int i = c->args_len - 1; i >= 0; i--) {
      free(c->args[i]);
    }
    free(c->args);
  }

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
  case  COMPILE_MODE_OBJECT: {
    if (!cmd_append(&cmd, "-c"))
      return false;
  } break;
  case COMPILE_MODE_BINARY:
    break;
   
  }

  if (c->args_len > 0) {
    for (int i = 0; i < c->args_len; i++) {
      if (!cmd_append_arg(&cmd, c->args[i]))
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
    char this_file[] = __FILE__;                                               \
    char *binary = argv[0];                                                    \
    if (is_newer(this_file, binary) || is_newer("./src/carp.h", binary)) {     \
      carp_log(CARP_LOG_INFO, "rebuilding carp");                              \
      carp_log(CARP_LOG_INFO, "cc -o carp carp.c");                            \
      system("cc -o carp carp.c");                                             \
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
#endif // IMPL_CARP
#endif // CARP_H_
