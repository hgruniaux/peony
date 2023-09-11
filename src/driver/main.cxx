#include "../codegen_llvm.hxx"
#include "../parser.hxx"
#include "../utils/diag.hxx"

#include "../options.hxx"

#include <clocale>
#include <cstdlib>

#include "ast/ast_printer.hxx"

#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>

#include <filesystem>

namespace fs = std::filesystem;

bool
compile_to(PSourceFile* p_source_file, const char* p_output_filename)
{
  PIdentifierTable identifier_table;
  identifier_table.register_keywords();

  PLexer lexer;
  lexer.identifier_table = &identifier_table;
  lexer.set_source_file(p_source_file);

  PContext& context = PContext::get_global();
  PParser parser(context, lexer);

  PAstTranslationUnit* ast = parser.parse();
  ast->dump(context);

  if (g_diag_context.diagnostic_count[P_DIAG_ERROR] == 0 && !g_options.opt_syntax_only) {
    PCodeGenLLVM codegen(context);
    codegen.codegen(ast->as<PAstTranslationUnit>());
    fs::create_directory("out");
    codegen.write_llvm_ir("out/" + p_source_file->get_filename() + ".ir");
    // codegen.optimize();
    codegen.write_llvm_ir("out/" + p_source_file->get_filename() + ".opt.ir");
    codegen.write_object_file("out/" + p_source_file->get_filename() + ".o");
  }

  return g_diag_context.diagnostic_count[P_DIAG_ERROR] != 0;
}

// Implemented in cmdline_parser.c
void
cmdline_parser(int p_argc, char* p_argv[]);

int
main(int p_argc, char* p_argv[])
{
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

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
