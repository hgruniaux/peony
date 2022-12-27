#include "ast_decl.hxx"
#include "ast_printer.hxx"

void
PDecl::dump(PContext& p_ctx)
{
  PAstPrinter printer(p_ctx);
  printer.visit(this);
}
