#include "ast_printer.hxx"

#include <cinttypes>
#include <cstdarg>
#include <cstdio>

PAstPrinter::PAstPrinter(PContext& p_ctx, std::FILE* p_output)
  : m_ctx(p_ctx)
  , m_output(p_output)
{
  m_use_colors = should_use_colors(m_output);
}

void
PAstPrinter::visit_null_stmt()
{
  print_stmt_header(nullptr, "{null stmt}");
  std::fputs("\n", m_output);
}

void
PAstPrinter::visit_translation_unit(const PAstTranslationUnit* p_node)
{
  m_src_file = p_node->p_src_file;

  print_stmt_header(p_node, "PAstTranslationUnit");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->decls);
  --m_indent;
}

void
PAstPrinter::visit_compound_stmt(const PAstCompoundStmt* p_node)
{
  print_stmt_header(p_node, "PAstCompoundStmt");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->stmts);
  --m_indent;
}

void
PAstPrinter::visit_let_stmt(const PAstLetStmt* p_node)
{
  print_stmt_header(p_node, "PAstLetStmt");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->var_decls);
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
PAstPrinter::visit_member_expr(const PAstMemberExpr* p_node)
{
  print_expr_header(p_node, "PAstMemberExpr");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->base_expr);

  m_elide_decls = true;
  visit(p_node->member);
  m_elide_decls = false;

  --m_indent;
}

void
PAstPrinter::visit_call_expr(const PAstCallExpr* p_node)
{
  print_expr_header(p_node, "PAstCallExpr");
  std::fputs("\n", m_output);

  ++m_indent;
  visit(p_node->callee);
  visit(p_node->args);
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
PAstPrinter::visit_struct_expr(const PAstStructExpr* p_node)
{
  print_expr_header(p_node, "PAstStructExpr");
  std::fputs("\n", m_output);

  ++m_indent;
  for (auto* field : p_node->get_fields()) {
    print_header(field, "PAstStructFieldExpr", true);
    print_ident_info(field->get_name());
    std::fputs("\n", m_output);

    ++m_indent;
    visit(field->get_expr());
    --m_indent;
  }
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
PAstPrinter::visit_null_decl()
{
  print_decl_header(nullptr, "{null decl}");
  std::fputs("\n", m_output);
}

void
PAstPrinter::visit_func_decl(const PFunctionDecl* p_node)
{
  print_decl_header(p_node, "PFunctionDecl");

  print_decl_body([this, p_node]() {
    visit(p_node->params);
    visit(p_node->get_body());
  });
}

void
PAstPrinter::visit_param_decl(const PParamDecl* p_node)
{
  print_decl_header(p_node, "PParamDecl");
  std::fputs("\n", m_output);
}

void
PAstPrinter::visit_var_decl(const PVarDecl* p_node)
{
  print_decl_header(p_node, "PVarDecl");

  print_decl_body([this, p_node]() {
    if (p_node->init_expr != nullptr)
      visit(p_node->init_expr);
  });
}

void
PAstPrinter::visit_struct_field_decl(const PStructFieldDecl* p_node)
{
  print_decl_header(p_node, "PStructFieldDecl");
  std::fputs("\n", m_output);
}

void
PAstPrinter::visit_struct_decl(const PStructDecl* p_node)
{
  print_decl_header(p_node, "PStructDecl");
  print_decl_body([this, p_node]() { visit(p_node->get_fields()); });
}

void
PAstPrinter::visit_null_ty()
{
  std::fprintf(m_output, "{null type}");
}

void
PAstPrinter::visit_ty(const PType* p_node)
{
  std::fprintf(m_output, "{unknown type}");
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
PAstPrinter::visit_tag_ty(const PTagType* p_node)
{
  auto* decl = p_node->get_decl();
  switch (decl->get_kind()) {
    case P_DK_STRUCT:
      std::fprintf(m_output, "struct");
      print_ident_info(decl->get_name(), false);
      break;
    default:
      assert(false && "unreachable");
      break;
  }
}

void
PAstPrinter::visit_unknown_ty(const PUnknownType* p_node)
{
  std::fprintf(m_output, "{unknown type:");
  print_ident_info(p_node->get_name(), false);
  std::fprintf(m_output, "}");
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

  if (m_show_addresses && p_ptr != nullptr) {
    color("33");
    std::fprintf(m_output, " %p", p_ptr);
    color_reset();
  }
}

void
PAstPrinter::print_decl_header(const PDecl* p_node, const char* p_name)
{
  print_header(p_node, p_name, false);

  if (p_node == nullptr)
    return;

  print_range(p_node->source_range);

  if (m_show_used && p_node->is_used()) {
    std::fprintf(m_output, " used");
  }

  print_ident_info(p_node->get_name());
  print_type(p_node->get_type());
}

void
PAstPrinter::print_stmt_header(const PAst* p_node, const char* p_name)
{
  print_header(p_node, p_name, true);

  if (p_node == nullptr)
    return;

  print_range(p_node->get_source_range());
}

void
PAstPrinter::print_expr_header(const PAstExpr* p_node, const char* p_name)
{
  print_header(p_node, p_name, true);

  if (p_node == nullptr)
    return;

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
PAstPrinter::print_ident_info(PIdentifierInfo* p_name, bool p_formatting)
{
  if (p_formatting)
    color("1");

  std::fprintf(m_output, " ");
  if (p_name != nullptr) {
    const auto spelling = p_name->get_spelling();
    std::fwrite(spelling.data(), sizeof(char), spelling.size(), m_output);
  } else {
    std::fprintf(m_output, "{unnamed}");
  }

  if (p_formatting)
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

#if __has_include(<unistd.h>)
#include <unistd.h>
#define IS_POSIX
#endif

bool
PAstPrinter::should_use_colors(std::FILE* p_output)
{
  // Actually we only use colors (by default) on POSIX systems when
  // p_output refers to a terminal (see isatty).

#ifdef IS_POSIX
  int file = fileno(p_output);
  return isatty(file);
#endif

  return false;
}
