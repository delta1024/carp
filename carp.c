#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

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

bool chechrebuild(int argc, char *argv[]) {

  char *file_name = __FILE__;

  if (is_newer(file_name, argv[0])) {
    printf("[BUILD] rebuilding carp\n.");
    system("cc -o carp carp.c");
    system("./carp");
    return true;
  }
  return false;
}

typedef struct {
  int count;
  int capacity;
  char *string;
} StringBuilder;

/**
 * @returns true for success; false for failure.
 */
bool sb_append(StringBuilder *sb, char *append) {
  int old_cap = sb->capacity;
  sb->capacity = old_cap + strlen(append) + 2; // add 1 for space.
  char *buff = malloc(sb->capacity);
  if (buff == NULL)
    return false;
  if (sb->count != 0) {
    sprintf(buff, "%s %s", sb->string, append);
    free(sb->string);
  } else {
    buff = strcpy(buff, append);
  }
  sb->string = buff;
  sb->count = sb->capacity;
  return true;
}
bool sb_append_many(StringBuilder *sb, char *append[], int len) {
  for (int i = 0; i < len; i++) {
    if (!sb_append(sb, append[i]))
      return false;
  }
  return true;
}

typedef struct {
  char *source_file; /**< Path to source file */
  char *name;        /**< binary name */
  int cflag_len;
  char **cflags;     /**< optional cflags */
  char *output_file; /**< output file path; if NULL build has yet to have taken
                        place. */
  bool binary;       /**< build as a binary or object */
} Compile;
bool compile_init(Compile *c, char *name, char *source_file, char **cflags,
                  int cflags_len, bool binary) {
  *c = (Compile){NULL, NULL, 0, NULL, NULL, false};

#define SRC "src/"
  c->source_file = malloc(strlen(source_file) + strlen(SRC) + 1);
  if (c->source_file == NULL)
    goto errdefer;
  if (sprintf(c->source_file, "%s%s", SRC, source_file) == -1)
    goto errdefer_sprintf;
#undef SRC
  c->name = malloc(strlen(name) + 1);
  if (c->name == NULL)
    goto errdefer_sprintf;
  c->name = memcpy(c->name, name, strlen(name));

  if (binary) {
    c->binary = true;
  }

  if (cflags != NULL) {
    c->cflags = malloc(sizeof(char *) * cflags_len);
    if (c->cflags == NULL)
      goto errdefer_name;
    for (int i = 0; i < cflags_len; i++) {
      c->cflags[i] = NULL;
    }

    for (int i = 0; i < cflags_len; i++) {
      c->cflags[i] = malloc(strlen(cflags[i]));
      if (c->cflags[i] == NULL)
        goto errdefer_arg_vals;
      c->cflag_len++;
      strcpy(c->cflags[i], cflags[i]);
    }
  }
  goto defer;

errdefer_arg_vals:
  if (c->cflags != NULL) {
    for (int i = c->cflag_len - 1; i >= 0; i--) {
      free(c->cflags[i]);
    }
    free(c->cflags);
  }
errdefer_name:
  free(c->name);
errdefer_sprintf:
  free(c->source_file);
errdefer:
  return false;

defer:
  return true;
}

bool compile_build(Compile *c) {
  DIR* build_dir = opendir("build");
  if (build_dir == NULL) {
    mkdir("build", 775);
  } else {
    closedir(build_dir);
  }
  StringBuilder sb = {0, 0, NULL};
  char *out_path = malloc(strlen("build/") + strlen(c->name));
  if (out_path == NULL) return false;
  sprintf(out_path, "build/%s", c->name);
  char *args[] = {
      "cc",
      "-o",
      out_path,
  };
  if (!sb_append_many(&sb, args, sizeof(args) / sizeof(char *)))
    return false;
  if (c->cflag_len > 0) {
    if (!sb_append_many(&sb, c->cflags, c->cflag_len))
      return false;
  }

  if (!sb_append(&sb, c->source_file))
    return false;

  printf("[BUILD] %s: %s\n", c->name, sb.string);
  if (system(sb.string) == -1)
    return false;
  c->output_file = out_path;
  return true;
}

int main(int argc, char *argv[]) {
  if (chechrebuild(argc, argv)) {
    exit(EXIT_SUCCESS);
  }
  Compile c;
  if (!compile_init(&c, "hello", "main.c", NULL, 0, true))
    exit(EXIT_FAILURE);
  compile_build(&c);
}
