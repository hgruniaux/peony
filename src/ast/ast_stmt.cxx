#include "ast_stmt.hxx"
#include "ast_printer.hxx"

void
PAst::dump(PContext& p_ctx)
{
  PAstPrinter printer(p_ctx);
  printer.visit(this);
}
