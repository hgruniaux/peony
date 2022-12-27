#ifndef PEONY_AST_PRINTER_HXX
#define PEONY_AST_PRINTER_HXX

#include "ast_visitor.hxx"

/// Utility class to dump, for debugging purposes, the AST to stdout.
///
/// This class is intended to dump AST statements or declarations but
/// not types (at least not alone).
class PAstPrinter : public PAstConstVisitor<PAstPrinter>
{
public:
  PAstPrinter(PContext& p_ctx, bool p_use_colors = true);

  void visit_translation_unit(const PAstTranslationUnit* p_node);
  void visit_compound_stmt(const PAstCompoundStmt* p_node);
  void visit_let_stmt(const PAstLetStmt* p_node);
  void visit_break_stmt(const PAstBreakStmt* p_node);
  void visit_continue_stmt(const PAstContinueStmt* p_node);
  void visit_return_stmt(const PAstReturnStmt* p_node);
  void visit_loop_stmt(const PAstLoopStmt* p_node);
  void visit_while_stmt(const PAstWhileStmt* p_node);
  void visit_if_stmt(const PAstIfStmt* p_node);

  void visit_bool_literal(const PAstBoolLiteral* p_node);
  void visit_int_literal(const PAstIntLiteral* p_node);
  void visit_float_literal(const PAstFloatLiteral* p_node);
  void visit_paren_expr(const PAstParenExpr* p_node);
  void visit_decl_ref_expr(const PAstDeclRefExpr* p_node);
  void visit_unary_expr(const PAstUnaryExpr* p_node);
  void visit_binary_expr(const PAstBinaryExpr* p_node);
  void visit_call_expr(const PAstCallExpr* p_node);
  void visit_cast_expr(const PAstCastExpr* p_node);
  void visit_l2rvalue_expr(const PAstL2RValueExpr* p_node);

  void visit_func_decl(const PFunctionDecl* p_node);
  void visit_param_decl(const PParamDecl* p_node);
  void visit_var_decl(const PVarDecl* p_node);

  void visit_ty(const PType* p_node);
  void visit_builtin_ty(const PType* p_node);
  void visit_paren_ty(const PParenType* p_node);
  void visit_func_ty(const PFunctionType* p_node);
  void visit_pointer_ty(const PPointerType* p_node);
  void visit_array_ty(const PArrayType* p_node);

private:
  void print(const char* p_format, ...);
  void print_header(const void* p_ptr, const char* p_name, bool p_is_stmt);
  void print_decl_header(const PDecl* p_node, const char* p_name);
  void print_stmt_header(const PAst* p_node, const char* p_name);
  void print_expr_header(const PAstExpr* p_node, const char* p_name);

  void print_range(PSourceRange p_src_range);
  void print_type(PType* p_type);
  void print_ident_info(PIdentifierInfo* p_name);

  void color(const char* p_color);
  void color_reset();

  template<class Fn>
  void print_decl_body(Fn p_fn)
  {
    if (m_elide_decls) {
      std::fputs(" [...]\n", m_output);
    } else {
      std::fputs("\n", m_output);

      ++m_indent;
      p_fn();
      --m_indent;
    }
  }

private:
  PContext& m_ctx;
  std::FILE* m_output;
  PSourceFile* m_src_file = nullptr;

  unsigned m_indent = 0;
  bool m_elide_decls = false;

  bool m_use_colors = true;

  // Should we show the memory address of each node?
  bool m_show_addresses = true;
  // Should we show source ranges and locations of each node?
  bool m_show_locations = true;
  // Should we show if declarations were used or not?
  bool m_show_used = true;
  // Should we show the types of each expression?
  bool m_show_types = true;
  // Should we show the value category (l or r value) of each expression?
  bool m_show_value_categories = true;
};

#endif // PEONY_AST_PRINTER_HXX
