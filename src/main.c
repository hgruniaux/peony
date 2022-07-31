#include "codegen_llvm.h"
#include "parser.h"

#include <stdio.h>

void
lex(const char* p_input)
{
  struct PIdentifierTable identifier_table;
  p_identifier_table_init(&identifier_table);

  struct PLexer lexer;
  lexer.identifier_table = &identifier_table;
  lexer.input = p_input;
  p_lexer_init(&lexer);

  struct PToken token;
  do {
    p_lex(&lexer, &token);
    p_token_dump(&token);
  } while (token.kind != P_TOK_EOF);

  p_lexer_destroy(&lexer);
  p_identifier_table_destroy(&identifier_table);
}

void
parse(const char* p_input)
{
  struct PBumpAllocator allocator;
  p_bump_init(&allocator);

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
  p_bump_destroy(&allocator);
}

int
main()
{
  p_bump_init(&p_global_bump_allocator);
  p_init_types();

  const char* code = "fn hello(x: bool): i64 {\n    return x as i64;\n}\n"
                     "fn main(): i32 {\n    return (hello(true) as i32) + 1;\n}";

  unsigned int lineno = 1;
  printf("%4d | ", lineno);
  for (const char* it = code; *it != '\0'; ++it) {
    if (*it == '\n') {
      lineno += 1;
      printf("\n%4d | ", lineno);
      continue;
    }

    putchar(*it);
  }
  puts("\n");

  parse(code);

  p_bump_destroy(&p_global_bump_allocator);
}
