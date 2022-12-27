#ifndef PEONY_AST_VISITOR_HXX
#define PEONY_AST_VISITOR_HXX

#include "ast.hxx"

namespace impl {
/// Common code for PAstVisitor and PAstConstVisitor.
template<class Derived, bool IsConst, class RetTy, class... Args>
class PAstVisitor
{
public:
  template<class T>
  using Ptr = std::conditional_t<IsConst, const T*, T*>;

#define DISPATCH(p_ty, p_func)                                                                                         \
  (static_cast<Derived*>(this)->p_func(p_node->template as<p_ty>(), std::forward<Args>(p_args)...))

  RetTy default_ret_value(Args&&... p_args) { return RetTy(); }

  // Statements
  RetTy visit_stmt(Ptr<PAst> p_node, Args&&... p_args) { return default_ret_value(std::forward<Args>(p_args)...); }
  RetTy visit_translation_unit(Ptr<PAstTranslationUnit> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_compound_stmt(Ptr<PAstCompoundStmt> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_let_stmt(Ptr<PAstLetStmt> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_break_stmt(Ptr<PAstBreakStmt> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_continue_stmt(Ptr<PAstContinueStmt> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_return_stmt(Ptr<PAstReturnStmt> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_loop_stmt(Ptr<PAstLoopStmt> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_while_stmt(Ptr<PAstWhileStmt> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_if_stmt(Ptr<PAstIfStmt> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }

  // Expressions
  RetTy visit_expr(Ptr<PAstExpr> p_node, Args&&... p_args) { return DISPATCH(PAst, visit_stmt); }
  RetTy visit_bool_literal(Ptr<PAstBoolLiteral> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_int_literal(Ptr<PAstIntLiteral> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_float_literal(Ptr<PAstFloatLiteral> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_paren_expr(Ptr<PAstParenExpr> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_decl_ref_expr(Ptr<PAstDeclRefExpr> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_unary_expr(Ptr<PAstUnaryExpr> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_binary_expr(Ptr<PAstBinaryExpr> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_call_expr(Ptr<PAstCallExpr> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_cast_expr(Ptr<PAstCastExpr> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }
  RetTy visit_l2rvalue_expr(Ptr<PAstL2RValueExpr> p_node, Args&&... p_args) { return DISPATCH(PAstExpr, visit_expr); }

  // Declarations
  RetTy visit_decl(Ptr<PDecl> p_node, Args&&... p_args) { return default_ret_value(std::forward<Args>(p_args)...); }
  RetTy visit_func_decl(Ptr<PFunctionDecl> p_node, Args&&... p_args) { return DISPATCH(PDecl, visit_decl); }
  RetTy visit_param_decl(Ptr<PParamDecl> p_node, Args&&... p_args) { return DISPATCH(PDecl, visit_decl); }
  RetTy visit_var_decl(Ptr<PVarDecl> p_node, Args&&... p_args) { return DISPATCH(PDecl, visit_decl); }

  // Types
  RetTy visit_ty(Ptr<PType> p_node, Args&&... p_args) { return default_ret_value(std::forward<Args>(p_args)...); }
  RetTy visit_builtin_ty(Ptr<PType> p_node, Args&&... p_args) { return DISPATCH(PType, visit_ty); }
  RetTy visit_paren_ty(Ptr<PParenType> p_node, Args&&... p_args) { return DISPATCH(PType, visit_ty); }
  RetTy visit_func_ty(Ptr<PFunctionType> p_node, Args&&... p_args) { return DISPATCH(PType, visit_ty); }
  RetTy visit_pointer_ty(Ptr<PPointerType> p_node, Args&&... p_args) { return DISPATCH(PType, visit_ty); }
  RetTy visit_array_ty(Ptr<PArrayType> p_node, Args&&... p_args) { return DISPATCH(PType, visit_ty); }

  RetTy visit(Ptr<PAst> p_node, Args&&... p_args)
  {
    assert(p_node != nullptr);
    switch (p_node->get_kind()) {
        // Statements
      case P_SK_TRANSLATION_UNIT:
        return DISPATCH(PAstTranslationUnit, visit_translation_unit);
      case P_SK_COMPOUND_STMT:
        return DISPATCH(PAstCompoundStmt, visit_compound_stmt);
      case P_SK_LET_STMT:
        return DISPATCH(PAstLetStmt, visit_let_stmt);
      case P_SK_BREAK_STMT:
        return DISPATCH(PAstBreakStmt, visit_break_stmt);
      case P_SK_CONTINUE_STMT:
        return DISPATCH(PAstContinueStmt, visit_continue_stmt);
      case P_SK_RETURN_STMT:
        return DISPATCH(PAstReturnStmt, visit_return_stmt);
      case P_SK_LOOP_STMT:
        return DISPATCH(PAstLoopStmt, visit_loop_stmt);
      case P_SK_WHILE_STMT:
        return DISPATCH(PAstWhileStmt, visit_while_stmt);
      case P_SK_IF_STMT:
        return DISPATCH(PAstIfStmt, visit_if_stmt);

      // Expressions
      case P_SK_BOOL_LITERAL:
        return DISPATCH(PAstBoolLiteral, visit_bool_literal);
      case P_SK_INT_LITERAL:
        return DISPATCH(PAstIntLiteral, visit_int_literal);
      case P_SK_FLOAT_LITERAL:
        return DISPATCH(PAstFloatLiteral, visit_float_literal);
      case P_SK_PAREN_EXPR:
        return DISPATCH(PAstParenExpr, visit_paren_expr);
      case P_SK_DECL_REF_EXPR:
        return DISPATCH(PAstDeclRefExpr, visit_decl_ref_expr);
      case P_SK_UNARY_EXPR:
        return DISPATCH(PAstUnaryExpr, visit_unary_expr);
      case P_SK_BINARY_EXPR:
        return DISPATCH(PAstBinaryExpr, visit_binary_expr);
      case P_SK_CALL_EXPR:
        return DISPATCH(PAstCallExpr, visit_call_expr);
      case P_SK_CAST_EXPR:
        return DISPATCH(PAstCastExpr, visit_cast_expr);
      case P_SK_L2RVALUE_EXPR:
        return DISPATCH(PAstL2RValueExpr, visit_l2rvalue_expr);

      default:
        assert(false && "unknown AST statement node m_kind");
        return default_ret_value(std::forward<Args>(p_args)...);
    }
  }

  RetTy visit(Ptr<PDecl> p_node, Args&&... p_args)
  {
    assert(p_node != nullptr);
    switch (p_node->get_kind()) {
      case P_DK_FUNCTION:
        return DISPATCH(PFunctionDecl, visit_func_decl);
      case P_DK_PARAM:
        return DISPATCH(PParamDecl, visit_param_decl);
      case P_DK_VAR:
        return DISPATCH(PVarDecl, visit_var_decl);
      default:
        assert(false && "unknown AST declaration node m_kind");
        return default_ret_value(std::forward<Args>(p_args)...);
    }
  }

  RetTy visit(Ptr<PType> p_node, Args&&... p_args)
  {
    assert(p_node != nullptr);
    switch (p_node->get_kind()) {
      case P_TK_PAREN:
        return DISPATCH(PParenType, visit_paren_ty);
      case P_TK_FUNCTION:
        return DISPATCH(PFunctionType, visit_func_ty);
      case P_TK_POINTER:
        return DISPATCH(PPointerType, visit_pointer_ty);
      case P_TK_ARRAY:
        return DISPATCH(PArrayType, visit_array_ty);
      default:
        return DISPATCH(PType, visit_builtin_ty);
    }
  }

  template<class T>
  void visit(PArrayView<T*> p_nodes, Args&&... p_args)
  {
    static_assert(std::is_base_of_v<PDecl, T> || std::is_base_of_v<PAst, T>, "T must inherits from PAst or PDecl");
    for (auto node : p_nodes)
      (void)visit(node, std::forward<Args>(p_args)...);
  }

  /// Helper method that calls visit on each elements.
  template<class InputIt>
  void visit_iter(InputIt p_begin, InputIt p_end, Args&&... p_args)
  {
    for (; p_begin != p_end; ++p_begin)
      (void)visit(*p_begin, std::forward<Args>(p_args)...);
  }

#undef DISPATCH
};
}

/// This class implements the visitor pattern and allows the traversal of the AST.
/// To use it, implement your own visitor by inhering from this class (providing
/// itself as the template argument, using the curiously recurring template pattern)
/// and override any of the methods that you need (visit_* functions where * is the
/// name of a AST node).
///
/// \see PAstConstVisitor
template<class Derived, class RetTy = void, class... Args>
using PAstVisitor = impl::PAstVisitor<Derived, /* IsConst= */ false, RetTy, Args...>;
/// \see PAstVisitor
template<class Derived, class RetTy = void, class... Args>
using PAstConstVisitor = impl::PAstVisitor<Derived, /* IsConst= */ true, RetTy, Args...>;

#endif // PEONY_AST_VISITOR_HXX
