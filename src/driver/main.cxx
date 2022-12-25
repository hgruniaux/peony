#include "../codegen_llvm.hxx"
#include "../parser.hxx"
#include "../utils/diag.hxx"

#include "../options.hxx"

#include <clocale>
#include <cstdlib>

bool
compile_to(PSourceFile* p_source_file, const char* p_output_filename)
{
  PIdentifierTable identifier_table;
  p_identifier_table_init(&identifier_table);
  p_identifier_table_register_keywords(&identifier_table);

  PLexer lexer;
  lexer.identifier_table = &identifier_table;
  lexer_init(&lexer);
  lexer_set_source_file(&lexer, p_source_file);

  PContext& context = PContext::get_global();
  PParser parser(context, lexer);

  PAst* ast = p_parse(&parser);
  if (g_diag_context.diagnostic_count[P_DIAG_ERROR] == 0 && !g_options.opt_syntax_only) {
    PCodegenLLVM codegen(context);
    codegen.opt_level = 0;

    p_cg_compile(&codegen, ast, p_output_filename);
  }

  lexer_destroy(&lexer);
  p_identifier_table_destroy(&identifier_table);
  return g_diag_context.diagnostic_count[P_DIAG_ERROR] != 0;
}

// Implemented in cmdline_parser.c
void
cmdline_parser(int p_argc, char* p_argv[]);

int
main(int p_argc, char* p_argv[])
{
  setlocale(LC_ALL, "C");

  cmdline_parser(p_argc, p_argv);
  if (g_diag_context.diagnostic_count[P_DIAG_ERROR] > 0)
    return EXIT_FAILURE;

  bool has_error = false;
  for (const auto& filename : g_options.input_files) {
    auto source_file = PSourceFile::open(filename);
    if (source_file == nullptr) {
      PDiag* d = diag(P_DK_err_fail_open_file);
      diag_add_arg_str(d, p_argv[1]);
      diag_flush(d);
      return EXIT_FAILURE;
    }

    has_error |= compile_to(source_file.get(), g_options.output_file);
  }

  return EXIT_FAILURE ? has_error : EXIT_SUCCESS;
}
