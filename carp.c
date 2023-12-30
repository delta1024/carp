#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *source_path; /**< The reletive path from the build directory of the
                        source file. */
  char *name;        /**< The object/binary name */
  bool bin;          /**< Binary or object flag */
  struct args {
    int len;
    char **ptr;
  } args;       /**< Compiliation arguments */
  char *output; /**< Output file for compilition. NULL if compilation has not
                   yet taken place. */
} Compile;

/** \relates Compile
 * Initialize a compile object.
 * @param[out] c            The Compile struct to be instantiated.
 * @param[in]  rel_src_path The path of the source file reletive to the src/
 * dir.
 * @param[in]  name         The name of the object to be created.
 * @param[in]  args         Optional additional compile arguments to be appended
 * to default compile args.
 * @param      arg_len      The lenth of the args; Is not checked if args is
 * NULL.
 * @param      bin          Flag for binary compilation or object compilation.
 * @returns                 0 on success, -1 on failure.
 */
int compile_init(Compile *c, char *rel_src_path, char *name, char *args[],
                 int arg_len, bool bin) {
  c->source_path = c->name = c->output = NULL;
  c->args.len = -1;
  c->args.ptr = NULL;
  c->bin = false;

#define SRC "src/"
  c->source_path =
      malloc(sizeof(char) * (strlen(rel_src_path) + strlen(SRC) + 1));
  if (c->source_path == NULL)
    goto errdefer;
  sprintf(c->source_path, "%s%s", SRC, rel_src_path);
#undef SRC

  c->name = malloc(strlen(name) * sizeof(char));
  if (c->name == NULL)
    goto errdefer_src_path;
  memcpy(c->name, name, strlen(name));

  if (args != NULL) {
    c->args.ptr = malloc(arg_len * sizeof(char *));
    if (c->args.ptr == NULL)
      goto errdefer_name;
    for (int i = 0; i < arg_len; i++) {
      c->args.ptr[i] = NULL;
    }

    for (int i = 0; i < arg_len; i++) {
      c->args.ptr[i] = malloc(sizeof(char) * strlen(args[i]));
      if (c->args.ptr[i] == NULL)
        goto errdefer_args;
      c->args.len = i;
      memcpy(c->args.ptr[i], args[i], strlen(args[i]));
    }
    c->args.len++;
  }

  if (bin)
    c->bin = true;

  goto defer;
errdefer_args:
  if (args != NULL) {
    for (int i = c->args.len - 1; i >= 0; i--) {
      free(c->args.ptr[i]);
    }
    free(c->args.ptr);
  }
errdefer_name:
  free(c->name);
errdefer_src_path:
  free(c->source_path);
errdefer:
  return -1;
defer:
  return 0;
}
void compile_free(Compile *c) {
  if (c->args.ptr != NULL) {
    for (int i = c->args.len - 1; i >= 0; i--) {
      free(c->args.ptr[i]);
    }
    free(c->args.ptr);
  }

  free(c->name);

  free(c->source_path);
}
int main(int argc, char *argv[]) {
  Compile c;
  if (compile_init(&c, "main.c", "hello", NULL, 0, true) == -1)
    return 1;
  printf("Compile{\n");
  printf("\tsource_path: %s,\n", c.source_path);
  printf("\tname: %s,\n", c.name);
  printf("}\n");
  compile_free(&c);
}
