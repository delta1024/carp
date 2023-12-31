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
  CARP_LOG_COMPILE,
} CARP_LOG_LEVEL;

typedef enum CARP_RESULT {
  CARP_RESULT_OK = 0,
  CARP_ERR_NOMEM = -1,
  CARP_ERR_COMPILE = -2,
  CARP_ERR_BAD_ARGS = -3,
} result_t;
static int carp_errno;

bool is_newer(char *to_check, char *older);
void carp_log(CARP_LOG_LEVEL level, const char *fmt, ...);

/************************************************************
 *                      sb.h                                *
 ************************************************************/

typedef struct {
  int cap;
  char *ptr;
} StringBuilder;

result_t sb_append(StringBuilder *sb, char *arg);
result_t sb_append_many(StringBuilder *sb, char **args, int args_len);

/************************************************************
 *                    compile.h                             *
 ************************************************************/
typedef struct Compile Compile;

typedef struct Deps {
  int count;
  int cap;
  Compile **ptr;
} Deps;

typedef struct Headers {
  int count;
  int cap;
  char **ptr;
} Headers;

struct Compile {
  char *name, *output_file, *source_file, **args;
  Deps deps;
  Headers headers;
  bool is_object;
  int args_len;
};

result_t compile_init(Compile *c, char *name, char *source_file, char *args[],
                      int arg_len, bool is_object);
bool compile_needs_rebuild(Compile *c);
result_t compile_run(Compile *c);
result_t deps_append(Deps* d, Compile *c);
result_t headers_append(Headers *d, char *path);

#ifdef CARP_IMPL
/************************************************************
 *                      carp.c                              *
 ************************************************************/

// NOLINTNEXTLINE(misc-definitions-in-headers)
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

void carp_log(CARP_LOG_LEVEL level, const char *fmt, ...) {
  char *log_message[] = {"[FATAL]", "[ERROR]", "[INFO]", "[COMPILE]"};
  fprintf(level > CARP_LOG_ERR ? stdout : stderr, "%s ", log_message[level]);
  va_list args;
  va_start(args, fmt);
  vfprintf(level > CARP_LOG_ERR ? stdout : stderr, fmt, args);
  va_end(args);
  putchar('\n');
}

/************************************************************
 *                       sb.c                               *
 ************************************************************/

// NOLINTNEXTLINE(misc-definitions-in-headers)
result_t sb_append(StringBuilder *sb, char *arg) {
  int arg_len = strlen(arg);
  int str_len;
  if (sb->ptr != NULL) {
    str_len = strlen(sb->ptr);
  } else {
    str_len = 0;
  }
  sb->ptr = (char *)realloc(sb->ptr, arg_len + str_len + 3);
  if (sb->ptr == NULL) {
    return CARP_ERR_NOMEM;
  }
  if (str_len != 0) {
    sb->ptr[str_len] = ' ';
    strcpy(&sb->ptr[str_len + 1], arg);
  } else {
    strcpy(sb->ptr, arg);
  }

  return CARP_RESULT_OK;
}

// NOLINTNEXTLINE(misc-definitions-in-headers)
result_t sb_append_many(StringBuilder *sb, char **args, int args_len) {
  result_t result;
  for (int i = 0; i < args_len; i++) {
    if ((result = sb_append(sb, args[i])) != CARP_RESULT_OK) {
      return result;
    }
  }
  return CARP_RESULT_OK;
}

/************************************************************
 *                    compile.c                             *
 ************************************************************/

// NOLINTNEXTLINE(misc-definitions-in-headers)
result_t deps_append(Deps *d, Compile *c) {
  if (d->cap < d->count + 1) {
    int old_cap = d->cap;
    d->cap = old_cap < 8 ? 8 : old_cap * 2;
    d->ptr = (Compile**)realloc(d->ptr, sizeof(Compile*) * d->cap);
    if (d->ptr == NULL){
      return CARP_ERR_NOMEM;
    }
  }
  d->ptr[d->count++] = c;
  return CARP_RESULT_OK;
}

// NOLINTNEXTLINE(misc-definitions-in-headers)
result_t headers_append(Headers *h, char *path) {
  if (h->cap < h->count + 1) {
    int old_cap = h->cap;
    h->cap = old_cap < 8 ? 8 : old_cap * 2;
    h->ptr = (char**)realloc(h->ptr, sizeof(char*) * h->cap);
    if (h->ptr == NULL){
      return CARP_ERR_NOMEM;
    }
  }
  h->ptr[h->count++] = path;
  return CARP_RESULT_OK;
}

// NOLINTNEXTLINE(misc-definitions-in-headers)
result_t compile_init(Compile *c, char *name, char *source_file, char *args[],
                      int arg_len, bool is_object) {
  result_t result;
  *c = (Compile){NULL, NULL, NULL, NULL, (Deps){0, 0, NULL}, (Headers){0,0,NULL}, false, 0};

  c->name = malloc(strlen(name) + 1);
  if (c->name == NULL) {
    result = CARP_ERR_NOMEM;
    goto no_dealloc;
  }
  strcpy(c->name, name);

  c->source_file = malloc(strlen(source_file) + 1);
  if (c->source_file == NULL) {
    result = CARP_ERR_NOMEM;
    goto free_name;
  }
  strcpy(c->source_file, source_file);

  if (is_object) {
    c->is_object = true;
    c->output_file = malloc(strlen(name) + 3 + strlen("build/"));
    if (c->output_file == NULL) {
      result = CARP_ERR_NOMEM;
      goto free_dest;
    }
    sprintf(c->output_file, "build/%s.o", name);
  } else {
    c->output_file = malloc(strlen(name) + 1 + strlen("build/"));
    if (c->output_file == NULL) {
      result = CARP_ERR_NOMEM;
      goto free_dest;
    }
    sprintf(c->output_file, "build/%s", name);
  }
  if (args != NULL) {
    c->args = malloc(sizeof(char *) * arg_len);
    if (c->args == NULL) {
      result = CARP_ERR_NOMEM;
      goto free_dest;
    }
    for (int i = 0; i < arg_len; i++) {
      char *buf = malloc(strlen(args[i]) + 1);
      if (buf == NULL) {
        result = CARP_ERR_NOMEM;
        goto free_args;
      }
      strcpy(c->args[i], buf);
      c->args_len++;
    }
  }

  result = CARP_RESULT_OK;
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

// NOLINTNEXTLINE(misc-definitions-in-headers)
bool compile_needs_rebuild(Compile *c) {
  if (!(access(c->output_file, F_OK) == 0 &&
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

// NOLINTNEXTLINE(misc-definitions-in-headers)
result_t compile_run(Compile *c) {

  DIR *build_dir = opendir("build");
  if (build_dir == NULL) {
    carp_log(CARP_LOG_INFO, "mkdir build");
    system("mkdir build");
  } else {
    closedir(build_dir);
  }

  if (!compile_needs_rebuild(c)) {
    return CARP_RESULT_OK;
  }

  result_t result;
  for (int i = 0; i < c->deps.count; i++) {
    if ((result = compile_run(c->deps.ptr[i])) != CARP_RESULT_OK) {
      return result;
    }
  }
  
  StringBuilder sb = {0, NULL};  
  char *args[] = {
      "cc",
      "-o",
      c->output_file,
  };
  
  if (sb_append_many(&sb, args, sizeof(args) / sizeof(char *)) !=
      CARP_RESULT_OK)
    return CARP_ERR_NOMEM;

  if (c->is_object) {
    if (sb_append(&sb, "-c") != CARP_RESULT_OK)
      return CARP_ERR_NOMEM;
  }
  
  if (c->args_len > 0) {
    if (sb_append_many(&sb, c->args, c->args_len) != CARP_RESULT_OK)
      return CARP_ERR_NOMEM;
  }

  if (sb_append(&sb, c->source_file) != CARP_RESULT_OK)
    return CARP_ERR_NOMEM;

  for (int i = 0; i < c->deps.count; i++) {
    if (sb_append(&sb, c->deps.ptr[i]->output_file) != CARP_RESULT_OK)
      return CARP_ERR_NOMEM;
  }

  carp_log(CARP_LOG_COMPILE, "%s", sb.ptr);
  if (system(sb.ptr) == -1)
    return CARP_ERR_COMPILE;

  return CARP_RESULT_OK;
}

/************************************************************
 *                      main.c                              *
 ************************************************************/

#define IMPL_REBUILD(argv, args_str)					\
  do {									\
  char this_file[] = __FILE__;						\
  char *binary = argv[0];						\
  if (is_newer(this_file, binary) || is_newer("./src/carp.h", binary)) { \
  carp_log(CARP_LOG_INFO, "rebuilding carp");				\
  carp_log(CARP_LOG_COMPILE, "cc -o carp carp.c");			\
  system("cc -o carp carp.c");						\
  if (args_str == NULL) {						\
    system("./carp");							\
  } else {								\
    StringBuilder sb = {0, NULL};					\
    char *args[] = {							\
      "./carp",								\
      args_str,								\
    };									\
    if (sb_append_many(&sb, args, sizeof(args) / sizeof(char*))		\
	!= CARP_RESULT_OK){						\
      return CARP_ERR_NOMEM;						\
    }									\
    system(sb.ptr);							\
  }									\
  return CARP_RESULT_OK;						\
  }									\
} while (false)

result_t build(int argc, char *argv[]);

// NOLINTNEXTLINE(misc-definitions-in-headers)
int main(int argc, char *argv[]) {
  switch (build(argc, argv)) {
  case CARP_ERR_NOMEM:
    carp_log(CARP_LOG_ERR, "OOM");
    exit(EXIT_FAILURE);
    break;
  case CARP_ERR_COMPILE: CARP_ERR_BAD_ARGS:
    exit(EXIT_FAILURE);
    break;


  default:
    break;
  }
}

#endif // CARP_IMPL

#endif // CARP_H
