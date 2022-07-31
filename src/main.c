#include "codegen_llvm.h"
#include "parser.h"
#include "utils/bump_allocator.h"
#include "utils/diag.h"

#include <assert.h>
#include <stdio.h>

void
parse(const char* p_input)
{
  struct PIdentifierTable identifier_table;
  p_identifier_table_init(&identifier_table);

  struct PLexer lexer;
  lexer.identifier_table = &identifier_table;
  lexer.input = p_input;
  p_lexer_init(&lexer);

  struct PParser parser;
  parser.lexer = &lexer;
  p_parser_init(&parser);

  PAst* ast = p_parse(&parser);
  if (parser.sema.error_count == 0) {
    struct PCodegenLLVM codegen;
    p_cg_init(&codegen);

    p_cg_compile(&codegen, ast);

    printf("=== LLVM IR: BEFORE OPTIMIZATION ===\n");
    p_cg_dump(&codegen);
    p_cg_optimize(&codegen, 3);
    printf("=== LLVM IR: AFTER OPTIMIZATION ===\n");
    p_cg_dump(&codegen);
    p_cg_destroy(&codegen);
  }

  p_lexer_destroy(&lexer);
  p_identifier_table_destroy(&identifier_table);
}

static const char*
read_file(const char* p_filename)
{
  FILE* file = fopen(p_filename, "rb");
  if (file == NULL) {
    error("failed to open file '%s'", p_filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long bufsize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* buffer = malloc(sizeof(char) * (bufsize + 1 /* NUL-terminated */));
  assert(buffer != NULL);

  fread(buffer, sizeof(char), bufsize, file);
  buffer[bufsize] = '\0';

  fclose(file);
  return buffer;
}

int
main(int p_argc, char* p_argv[])
{
  if (p_argc != 2) {
    printf("USAGE:\n");
    printf("    %s filename\n", p_argv[0]);
    return EXIT_FAILURE;
  }

  const char* code = read_file(p_argv[1]);
  if (code == NULL)
    return EXIT_FAILURE;

  p_bump_init(&p_global_bump_allocator);
  p_init_types();

  parse(code);

  free(code);
  p_bump_destroy(&p_global_bump_allocator);
  return EXIT_SUCCESS;
}
