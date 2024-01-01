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
    if (!cmd_append_arg(cmd, arg))
      return false;
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
  carp_log(CARP_LOG_COMMAND, sb.ptr);
  /* if (system(sb.ptr) < 0) { */

  /*   perror("cmd_run"); */
  /*   return false; */
  /* } */
  free(sb.ptr);
  return true;
}

int main(int argc, char *argv[]) {
  Cmd cmd = {0};

  if (!cmd_append(&cmd, "cc", "-o", "carp", "carp.c")){
    carp_perror("cmd_append");
    return 1;
  }
  if (!cmd_run(&cmd))
    return 1;
  if (cmd.items != NULL)
    free(cmd.items);
}
