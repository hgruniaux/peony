#ifndef PEONY_CODEGEN_LLVM_HXX
#define PEONY_CODEGEN_LLVM_HXX

#include "ast/ast_visitor.hxx"

namespace llvm {
class Module;
}

/// LLVM code generator.
class PCodeGenLLVM : public PAstConstVisitor<PCodeGenLLVM, void*>
{
public:
  PCodeGenLLVM(PContext& p_ctx);
  ~PCodeGenLLVM();

  bool codegen(PAstTranslationUnit* p_ast);
  void optimize();
  bool write_llvm_ir(const std::string& p_filename);
  bool write_object_file(const std::string& p_filename);

  // Statements:
  void* visit_translation_unit(const PAstTranslationUnit* p_node);
  void* visit_compound_stmt(const PAstCompoundStmt* p_node);
  void* visit_let_stmt(const PAstLetStmt* p_node);
  void* visit_break_stmt(const PAstBreakStmt* p_node);
  void* visit_continue_stmt(const PAstContinueStmt* p_node);
  void* visit_return_stmt(const PAstReturnStmt* p_node);
  void* visit_loop_stmt(const PAstLoopStmt* p_node);
  void* visit_while_stmt(const PAstWhileStmt* p_node);
  void* visit_if_stmt(const PAstIfStmt* p_node);
  void* visit_assert_stmt(const PAstAssertStmt* p_node);

  // Expressions:
  void* visit_bool_literal(const PAstBoolLiteral* p_node);
  void* visit_int_literal(const PAstIntLiteral* p_node);
  void* visit_float_literal(const PAstFloatLiteral* p_node);
  void* visit_paren_expr(const PAstParenExpr* p_node);
  void* visit_decl_ref_expr(const PAstDeclRefExpr* p_node);
  void* visit_unary_expr(const PAstUnaryExpr* p_node);
  void* visit_binary_expr(const PAstBinaryExpr* p_node);
  void* visit_member_expr(const PAstMemberExpr* p_node);
  void* visit_call_expr(const PAstCallExpr* p_node);
  void* visit_cast_expr(const PAstCastExpr* p_node);
  void* visit_struct_expr(const PAstStructExpr* p_node);
  void* visit_l2rvalue_expr(const PAstL2RValueExpr* p_node);

  // Declarations:
  void* visit_func_decl(const PFunctionDecl* p_node);
  void* visit_var_decl(const PVarDecl* p_node);

private:
  /// Emits code to implement the lazy binary operators '&&' and '||' (depending
  /// on the parameter p_is_and).
  void* emit_log_and(PAstExpr* p_lhs, PAstExpr* p_rhs, bool p_is_and);
  void* try_codegen_constant_log_and(PAstExpr* p_lhs, PAstExpr* p_rhs, bool p_is_and);
  void* emit_trivial_bin_op(PType* p_type, void* p_llvm_lhs, void* p_llvm_rhs, PAstBinaryOp p_op);

private:
  struct D;
  std::unique_ptr<D> m_d;
};

#endif // PEONY_CODEGEN_LLVM_HXX
