#ifndef PEONY_SEMA_HXX
#define PEONY_SEMA_HXX

#include "ast/ast.hxx"
#include "context.hxx"
#include "scope.hxx"
#include "token.hxx"
#include "utils/diag.hxx"

#include <stack>

/// The semantic analyzer.
///
/// In Peony, the semantic analyzer is called by a callback-based API which is
/// in charge, on the one hand, to check the semantic of the program and, on the
/// other hand, to effectively build the AST. These callbacks can be called by
/// anyone, yet in Peony they are called by the parser.
class PSema
{
public:
  explicit PSema(PContext& context);
  ~PSema();

  void push_scope(PScopeFlags p_flags = P_SF_NONE);
  void pop_scope();

  [[nodiscard]] PSymbol* lookup(PIdentifierInfo* p_name) const;
  [[nodiscard]] PSymbol* local_lookup(PIdentifierInfo* p_name) const;

  /// Lookups for a tag type with the given name (e.g. a struct or alias type).
  /// Builtin types that are identified by a keyword are not recognized by this
  /// function!
  [[nodiscard]] PType* lookup_type(PLocalizedIdentifierInfo p_name,
                                   PDiagKind p_diag = P_DK_err_type_unknown,
                                   bool p_return_unknown = true) const;

  [[nodiscard]] PAstBoolLiteral* act_on_bool_literal(bool p_value, PSourceRange p_src_range = {});

  /// Returns the type corresponding to the given integer literal suffix.
  /// If p_suffix is P_ILS_NO_SUFFIX, the smallest yet greater than `i32`
  /// integer type that can store `p_value` is returned.
  [[nodiscard]] PType* get_type_for_int_literal_suffix(PIntLiteralSuffix p_suffix, uintmax_t p_value = 0) const;
  /// Returns `true` if the given type is big enough to store `p_value`.
  [[nodiscard]] bool is_int_type_big_enough(uintmax_t p_value, PType* p_type) const;
  [[nodiscard]] PAstIntLiteral* act_on_int_literal(uintmax_t p_value,
                                                   PIntLiteralSuffix p_suffix,
                                                   PSourceRange p_src_range = {});

  /// Same as get_type_for_int_literal_suffix() but for a float literal.
  [[nodiscard]] PType* get_type_for_float_literal_suffix(PFloatLiteralSuffix p_suffix, double p_value = 0.0) const;
  [[nodiscard]] PAstFloatLiteral* act_on_float_literal(double p_value,
                                                       PFloatLiteralSuffix p_suffix,
                                                       PSourceRange p_src_range = {});

  [[nodiscard]] PAstParenExpr* act_on_paren_expr(PAstExpr* p_sub_expr, PSourceRange p_src_range = {});

  [[nodiscard]] PAstLetStmt* act_on_let_stmt(PArrayView<PVarDecl*> p_decls, PSourceRange p_src_range = {});

  [[nodiscard]] PAstBreakStmt* act_on_break_stmt(PSourceRange p_src_range = {}, PSourceRange p_break_key_range = {});
  [[nodiscard]] PAstContinueStmt* act_on_continue_stmt(PSourceRange p_src_range = {},
                                                       PSourceRange p_cont_key_range = {});

  /// Returns `true` if the given type is compatible (that is canonically equivalent or
  /// convertible to) the return type of the current function.
  [[nodiscard]] bool is_compatible_with_ret_ty(PType* p_type) const;
  [[nodiscard]] PAstReturnStmt* act_on_return_stmt(PAstExpr* p_ret_expr, PSourceRange p_src_range = {});

  /// Checks (and emits diagnostics) a given expression used as a condition
  /// like in a `if` or `while` statement.
  void check_condition_expr(PAstExpr* p_cond_expr) const;

  /// To be called just before analysing a while statement body but after analysing
  /// the condition expression. However, act_on_while_stmt() must be called after
  /// the body analysis and thus after this function.
  void act_before_while_stmt_body();
  [[nodiscard]] PAstWhileStmt* act_on_while_stmt(PAstExpr* p_cond_expr, PAst* p_body, PSourceRange p_src_range = {});

  void act_before_loop_stmt_body();
  [[nodiscard]] PAstLoopStmt* act_on_loop_stmt(PAst* p_body, PSourceRange p_src_range = {});

  [[nodiscard]] PAstIfStmt* act_on_if_stmt(PAstExpr* p_cond_expr,
                                           PAst* p_then_stmt,
                                           PAst* p_else_stmt,
                                           PSourceRange p_src_range = {});

  [[nodiscard]] PAstAssertStmt* act_on_assert_stmt(PAstExpr* p_cond_expr, PSourceRange p_src_range = {});

  [[nodiscard]] PAstDeclRefExpr* act_on_decl_ref_expr(PIdentifierInfo* p_name, PSourceRange p_src_range = {});

  [[nodiscard]] PAstUnaryExpr* act_on_unary_expr(PAstExpr* p_sub_expr,
                                                 PAstUnaryOp p_opcode,
                                                 PSourceRange p_src_range = {});
  [[nodiscard]] PAstBinaryExpr* act_on_binary_expr(PAstExpr* p_lhs,
                                                   PAstExpr* p_rhs,
                                                   PAstBinaryOp p_opcode,
                                                   PSourceRange p_src_range = {},
                                                   PSourceLocation p_op_loc = {});

  PAstMemberExpr* act_on_member_expr(PAstExpr* p_base_expr,
                                     PLocalizedIdentifierInfo p_member_name,
                                     PSourceRange p_src_range = {},
                                     PSourceLocation p_dot_loc = {});

  /// Checks the arguments used in a call expression to a callee of the given type.
  /// The param func_decl is optional and it is used for better diagnostics.
  void check_call_args(PFunctionType* p_callee_ty,
                       PArrayView<PAstExpr*> p_args,
                       PFunctionDecl* p_func_decl = nullptr,
                       PSourceLocation p_lparen_loc = {});
  [[nodiscard]] PAstCallExpr* act_on_call_expr(PAstExpr* p_callee,
                                               PArrayView<PAstExpr*> p_args,
                                               PSourceRange p_src_range = {},
                                               PSourceLocation p_lparen_loc = {});

  [[nodiscard]] PAstCastExpr* act_on_cast_expr(PAstExpr* p_sub_expr,
                                               PType* p_target_ty,
                                               PSourceRange p_src_range = {},
                                               PSourceLocation p_as_loc = {});

  [[nodiscard]] PStructDecl* resolve_struct_expr_name(PLocalizedIdentifierInfo p_name);
  [[nodiscard]] PAstStructFieldExpr* act_on_struct_field_expr(PStructDecl* p_struct_decl,
                                                              PLocalizedIdentifierInfo p_name,
                                                              PAstExpr* p_expr);
  [[nodiscard]] PAstStructExpr* act_on_struct_expr(PStructDecl* p_struct_decl,
                                                   PArrayView<PAstStructFieldExpr*> p_fields,
                                                   PSourceRange p_src_range = {});

  [[nodiscard]] PAstTranslationUnit* act_on_translation_unit(PArrayView<PDecl*> p_decls);

  [[nodiscard]] PVarDecl* act_on_var_decl(PType* p_type,
                                          PLocalizedIdentifierInfo p_name,
                                          PAstExpr* p_init_expr,
                                          PSourceRange p_src_range = {});
  [[nodiscard]] PParamDecl* act_on_param_decl(PType* p_type,
                                              PLocalizedIdentifierInfo p_name,
                                              PSourceRange p_src_range = {});

  void begin_func_decl_analysis(PFunctionDecl* p_decl);
  void end_func_decl_analysis();
  void check_func_abi(std::string_view abi, PSourceRange p_src_range = {});
  [[nodiscard]] PFunctionDecl* act_on_func_decl(PLocalizedIdentifierInfo p_name,
                                                PType* p_ret_ty,
                                                PArrayView<PParamDecl*> p_params,
                                                PSourceLocation p_lparen_loc = {});

  [[nodiscard]] PStructFieldDecl* act_on_struct_field_decl(PType* p_type,
                                                           PLocalizedIdentifierInfo p_name,
                                                           PSourceRange p_src_range = {});

  void check_struct_fields(PArrayView<PStructFieldDecl*> p_fields);
  [[nodiscard]] PStructDecl* act_on_struct_decl(PLocalizedIdentifierInfo p_name,
                                                PArrayView<PStructFieldDecl*> p_fields,
                                                PSourceRange p_src_range = {});

private:
  /// Common code for act_before_while_stmt_body() and act_before_loop_stmt_body().
  void act_before_loop_body_common();

  /// Converts a l-value expression to a r-value one. If p_expr is already
  /// a r-value then it is returned as is.
  PAstExpr* convert_to_rvalue(PAstExpr* p_expr);

  /// Try to find the referenced declaration by the given expression.
  /// For `((foo))` it will return the declaration referenced by `foo`.
  /// If we can not find such a declaration, null is returned.
  PDecl* try_get_ref_decl(PAstExpr* p_expr);

  template<class T>
  PArrayView<T> make_array_view_copy(PArrayView<T> p_other)
  {
    auto* data = m_context.alloc_object<T>(p_other.size());
    std::copy(p_other.begin(), p_other.end(), data);
    return { data, p_other.size() };
  }

  PScope* find_nearest_scope_with_flag(PScopeFlags p_flag)
  {
    PScope* scope = m_current_scope;
    while (scope != nullptr) {
      if (scope->flags & p_flag)
        return scope;

      scope = scope->parent_scope;
    }

    return nullptr;
  }

  PContext& m_context;
  PScope* m_current_scope = nullptr;
  PFunctionType* m_curr_func_type;
};

#endif // PEONY_SEMA_HXX
