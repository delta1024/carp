#ifndef CARP_H_
#define CARP_H_

#include <dirent.h>
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
typedef enum CARP_RESULT {
  CARP_RESULT_OK = 0,
  CARP_ERR_NOMEM = -1,
} result_t;
static int carp_errno;

bool is_newer(char *to_check, char *older);

/************************************************************
 *                      sb.h                                *
 ************************************************************/

typedef struct {
  int cap;
  char *ptr;
} StringBuilder;

result_t sb_append(StringBuilder *sb, char *arg);
result_t sb_append_many(StringBuilder *sb, char **args, int args_len);


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
  sb->ptr = (char*)realloc(sb->ptr, arg_len + str_len + 3);
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
 *                      main.c                              *
 ************************************************************/


#define IMPL_REBUILD(argv)						\
  do {									\
    char this_file[] = __FILE__;					\
    char *binary = argv[0];						\
    if (is_newer(this_file, binary)					\
	|| is_newer("./src/carp.h", binary)) {				\
      system("cc -o carp carp.c");					\
      system("./carp");							\
      return CARP_RESULT_OK;						\
    }									\
  } while(false) 

result_t build(int argc, char *argv[]);

// NOLINTNEXTLINE(misc-definitions-in-headers)
int main(int argc, char *argv[]) {
  switch (build(argc, argv)) {
  case CARP_ERR_NOMEM:
    fprintf(stderr, "OOM\n");
    exit(EXIT_FAILURE);
    break;
  default:
    break;
  }
}

#endif // CARP_IMPL

#endif // CARP_H
