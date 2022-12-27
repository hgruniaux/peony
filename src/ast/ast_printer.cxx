#include "ast_printer.hxx"

#include <cinttypes>
#include <cstdarg>
#include <cstdio>

PAstPrinter::PAstPrinter(PContext& p_ctx, bool p_use_colors)
  : m_ctx(p_ctx)
  , m_output(stderr)
{
}

void
PAstPrinter::visit_translation_unit(const PAstTranslationUnit* p_node)
{
  m_src_file = p_node->p_src_file;

  print_stmt_header(p_node, "PAstTranslationUnit");
  std::fputs("\n", m_output);

  ++m_indent;
  visit_iter(p_node->decls, p_node->decls + p_node->decl_count);
  --m_indent;
}

void
PAstPrinter::visit_compound_stmt(const PAstCompoundStmt* p_node)
{
  print_stmt_header(p_node, "PAstCompoundStmt");
  std::fputs("\n", m_output);

  ++m_indent;
  visit_iter(p_node->stmts, p_node->stmts + p_node->stmt_count);
  --m_indent;
}

void
PAstPrinter::visit_let_stmt(const PAstLetStmt* p_node)
{
  print_stmt_header(p_node, "PAstLetStmt");
  std::fputs("\n", m_output);

  ++m_indent;
  visit_iter(p_node->var_decls, p_node->var_decls + p_node->var_decl_count);
  --m_indent;
}

void
PAstPrinter::visit_break_stmt(const PAstBreakStmt* p_node)
{
  print_stmt_header(p_node, "PAstBreakStmt");
  std::fputs("\n", m_output);
}

void
PAstPrinter::visit_continue_stmt(const PAstContinueStmt* p_node)
{
  print_stmt_header(p_node, "PAstContinueStmt");
  std::fputs("\n", m_output);
}

void
PAstPrinter::visit_return_stmt(const PAstReturnStmt* p_node)
{
  print_stmt_header(p_node, "PAstReturnStmt");
  std::fputs("\n", m_output);

  if (p_node->ret_expr != nullptr) {
    ++m_indent;
    visit(p_node->ret_expr);
    --m_indent;
  }
}

void
PAstPrinter::visit_loop_stmt(const PAstLoopStmt* p_node)
{
  print_stmt_header(p_node, "PAstLoopStmt");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->body_stmt);
  --m_indent;
}

void
PAstPrinter::visit_while_stmt(const PAstWhileStmt* p_node)
{
  print_stmt_header(p_node, "PAstWhileStmt");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->cond_expr);
  visit(p_node->body_stmt);
  --m_indent;
}

void
PAstPrinter::visit_if_stmt(const PAstIfStmt* p_node)
{
  print_stmt_header(p_node, "PAstIfStmt");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->cond_expr);
  visit(p_node->then_stmt);
  if (p_node->else_stmt != nullptr)
    visit(p_node->else_stmt);
  --m_indent;
}

void
PAstPrinter::visit_bool_literal(const PAstBoolLiteral* p_node)
{
  print_expr_header(p_node, "PAstBoolLiteral");
  std::fprintf(m_output, " (value = %s)\n", p_node->value ? "true" : "false");
}

void
PAstPrinter::visit_int_literal(const PAstIntLiteral* p_node)
{
  print_expr_header(p_node, "PAstIntLiteral");
  std::fprintf(m_output, " (value = %" PRIuMAX ")\n", p_node->value);
}

void
PAstPrinter::visit_float_literal(const PAstFloatLiteral* p_node)
{
  print_expr_header(p_node, "PAstFloatLiteral");
  std::fprintf(m_output, " (value = %f)\n", p_node->value);
}

void
PAstPrinter::visit_paren_expr(const PAstParenExpr* p_node)
{
  print_expr_header(p_node, "PAstParenExpr");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->sub_expr);
  --m_indent;
}

void
PAstPrinter::visit_decl_ref_expr(const PAstDeclRefExpr* p_node)
{
  print_expr_header(p_node, "PAstDeclRefExpr");
  std::fputs("\n", m_output);

  ++m_indent;
  m_elide_decls = true;
  visit(p_node->decl);
  m_elide_decls = false;
  --m_indent;
}

void
PAstPrinter::visit_unary_expr(const PAstUnaryExpr* p_node)
{
  print_expr_header(p_node, "PAstUnaryExpr");
  std::fprintf(m_output, " '%s'\n", p_get_spelling(p_node->opcode));

  ++m_indent;
  visit(p_node->sub_expr);
  --m_indent;
}

void
PAstPrinter::visit_binary_expr(const PAstBinaryExpr* p_node)
{
  print_expr_header(p_node, "PAstBinaryExpr");
  std::fprintf(m_output, " '%s'\n", p_get_spelling(p_node->opcode));

  ++m_indent;
  visit(p_node->lhs);
  visit(p_node->rhs);
  --m_indent;
}

void
PAstPrinter::visit_call_expr(const PAstCallExpr* p_node)
{
  print_expr_header(p_node, "PAstCallExpr");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->callee);
  visit_iter(p_node->args, p_node->args + p_node->arg_count);
  --m_indent;
}

void
PAstPrinter::visit_cast_expr(const PAstCastExpr* p_node)
{
  print_expr_header(p_node, "PAstCastExpr");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->sub_expr);
  --m_indent;
}

void
PAstPrinter::visit_l2rvalue_expr(const PAstL2RValueExpr* p_node)
{
  print_expr_header(p_node, "PAstL2RValueExpr");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->sub_expr);
  --m_indent;
}

void
PAstPrinter::visit_func_decl(const PFunctionDecl* p_node)
{
  print_decl_header(p_node, "PFunctionDecl");
  print_type(p_node->type);

  print_decl_body([this, p_node]() {
    visit_iter(p_node->params, p_node->params + p_node->param_count);
    visit(p_node->body);
  });
}

void
PAstPrinter::visit_param_decl(const PParamDecl* p_node)
{
  print_decl_header(p_node, "PParamDecl");
  print_type(p_node->type);
  std::fputs("\n", m_output);
}

void
PAstPrinter::visit_var_decl(const PVarDecl* p_node)
{
  print_decl_header(p_node, "PVarDecl");
  print_type(p_node->type);

  print_decl_body([this, p_node]() {
    if (p_node->init_expr != nullptr)
      visit(p_node->init_expr);
  });
}

void
PAstPrinter::visit_ty(const PType* p_node)
{
  std::fprintf(m_output, "{unknown}");
}

void
PAstPrinter::visit_builtin_ty(const PType* p_node)
{
  switch (p_node->get_kind()) {
#define TYPE(p_kind)
#define BUILTIN_TYPE(p_kind, p_spelling)                                                                               \
  case p_kind:                                                                                                         \
    std::fprintf(m_output, p_spelling);                                                                                \
    break;
#include "type.def"
    default:
      assert(false && "not a builtin type");
      break;
  }
}

void
PAstPrinter::visit_paren_ty(const PParenType* p_node)
{
  std::fprintf(m_output, "(");
  visit(p_node->get_sub_type());
  std::fprintf(m_output, ")");
}

void
PAstPrinter::visit_func_ty(const PFunctionType* p_node)
{
  std::fprintf(m_output, "fn (");

  for (size_t i = 0; i < p_node->get_param_count(); ++i) {
    visit(p_node->get_params()[i]);
    if (i != p_node->get_param_count() - 1)
      std::fprintf(m_output, ", ");
  }

  std::fprintf(m_output, ")");

  if (!p_node->get_ret_ty()->is_void_ty()) {
    std::fprintf(m_output, " -> ");
    visit(p_node->get_ret_ty());
  }
}

void
PAstPrinter::visit_pointer_ty(const PPointerType* p_node)
{
  std::fprintf(m_output, "*");
  visit(p_node->get_element_ty());
}

void
PAstPrinter::visit_array_ty(const PArrayType* p_node)
{
  std::fprintf(m_output, "[");
  visit(p_node->get_element_ty());
  std::fprintf(m_output, "; %zd]", p_node->get_num_elements());
}

void
PAstPrinter::print(const char* p_format, ...)
{
  unsigned indent_level = m_indent;
  while (indent_level-- > 0)
    std::fprintf(m_output, "  ");

  std::va_list ap;
  va_start(ap, p_format);
  std::vfprintf(m_output, p_format, ap);
  va_end(ap);
}

void
PAstPrinter::print_header(const void* p_ptr, const char* p_name, bool p_is_expr)
{
  color(p_is_expr ? "95" : "92");
  print("%s", p_name, p_ptr);
  color_reset();

  if (m_show_addresses) {
    color("33");
    std::fprintf(m_output, " %p", p_ptr);
    color_reset();
  }
}

void
PAstPrinter::print_decl_header(const PDecl* p_node, const char* p_name)
{
  print_header(p_node, p_name, false);
  print_range(p_node->source_range);

  if (m_show_used && p_node->used) {
    std::fprintf(m_output, " used");
  }

  print_ident_info(p_node->name);
}

void
PAstPrinter::print_stmt_header(const PAst* p_node, const char* p_name)
{
  print_header(p_node, p_name, true);
  print_range(p_node->get_source_range());
}

void
PAstPrinter::print_expr_header(const PAstExpr* p_node, const char* p_name)
{
  print_header(p_node, p_name, true);
  print_range(p_node->get_source_range());

  if (m_show_value_categories && p_node->is_lvalue()) {
    color("34");
    std::fprintf(m_output, " lvalue");
    color_reset();
  }

  print_type(p_node->get_type(m_ctx));
}

void
PAstPrinter::print_range(PSourceRange p_src_range)
{
  if (!m_show_locations || m_src_file == nullptr)
    return;

  std::fputs(" <", m_output);
  color("33");
  if (p_src_range.begin == p_src_range.end) {
    // Really a source location.
    uint32_t lineno, colno;
    p_source_location_get_lineno_and_colno(m_src_file, p_src_range.begin, &lineno, &colno);
    std::fprintf(m_output, "%d:%d", lineno, colno);
  } else {
    uint32_t start_lineno, start_colno;
    uint32_t end_lineno, end_colno;
    p_source_location_get_lineno_and_colno(m_src_file, p_src_range.begin, &start_lineno, &start_colno);
    p_source_location_get_lineno_and_colno(m_src_file, p_src_range.end, &end_lineno, &end_colno);
    std::fprintf(m_output, "%d:%d", start_lineno, start_colno);
    color_reset();
    std::fputs(" -> ", m_output);
    color("33");
    std::fprintf(m_output, "%d:%d", end_lineno, end_colno);
  }
  color_reset();
  std::fputs(">", m_output);
}

void
PAstPrinter::print_type(PType* p_type)
{
  if (!m_show_types)
    return;

  color("32");
  std::fprintf(m_output, " '");
  visit(p_type);
  std::fprintf(m_output, "'");
  color_reset();
}

void
PAstPrinter::print_ident_info(PIdentifierInfo* p_name)
{
  color("1");
  std::fprintf(m_output, " ");
  if (p_name != nullptr) {
    std::fwrite(p_name->spelling, sizeof(char), p_name->spelling_len, m_output);
  } else {
    std::fprintf(m_output, "unnamed");
  }
  color_reset();
}

void
PAstPrinter::color(const char* p_color)
{
  if (!m_use_colors)
    return;

  std::fprintf(m_output, "\x1b[%sm", p_color);
}
void
PAstPrinter::color_reset()
{
  color("0");
}
