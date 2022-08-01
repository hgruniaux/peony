#include "codegen_llvm.h"
#include "parser.h"
#include "utils/bump_allocator.h"
#include "utils/diag.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
parse(PSourceFile* p_source_file)
{
  struct PIdentifierTable identifier_table;
  p_identifier_table_init(&identifier_table);

  PLexer lexer;
  lexer.identifier_table = &identifier_table;
  p_lexer_init(&lexer, p_source_file);

  struct PParser parser;
  parser.lexer = &lexer;
  p_parser_init(&parser);

  PAst* ast = p_parse(&parser);
  if (parser.sema.error_count == 0) {
    struct PCodegenLLVM codegen;
    p_cg_init(&codegen);
    codegen.opt_level = 0;

    p_cg_compile(&codegen, ast);

    p_cg_dump(&codegen);
    p_cg_destroy(&codegen);
  }

  p_lexer_destroy(&lexer);
  p_identifier_table_destroy(&identifier_table);
}

#include "utils/diag_formatter.h"

int
main(int p_argc, char* p_argv[])
{
  if (p_argc < 2) {
    printf("USAGE:\n");
    printf("    %s filename\n", p_argv[0]);
    return EXIT_FAILURE;
  }

  for (int i = 1; i < p_argc; ++i) {
    if (strcmp(p_argv[i], "-verify") == 0) {
      g_verify_mode_enabled = true;
      g_enable_ansi_colors = false;
    } else if (strcmp(p_argv[i], "-Wfatal-errors") == 0) {
      g_diag_context.fatal_errors = true;
    }
  }

  PSourceFile* source_file = p_source_file_open(p_argv[1]);
  if (source_file == NULL) {
    PDiag* d = diag(P_DK_err_fail_open_file);
    diag_add_arg_str(d, p_argv[1]);
    diag_flush(d);
    return EXIT_FAILURE;
  }

  p_bump_init(&p_global_bump_allocator);
  p_init_types();

  parse(source_file);

  if (g_verify_mode_enabled)
    verify_finalize();

  p_source_file_close(source_file);
  p_bump_destroy(&p_global_bump_allocator);
  return EXIT_SUCCESS;
}
