#ifndef PEONY_AST_EXPR_HXX
#define PEONY_AST_EXPR_HXX

#include "ast_decl.hxx"
#include "ast_stmt.hxx"

/* --------------------------------------------------------
 * Expressions
 */

/// \brief A value category (either lvalue or rvalue).
enum PValueCategory
{
  P_VC_LVALUE,
  P_VC_RVALUE
};

class PAstExpr : public PAst
{
public:
  [[nodiscard]] PAstExpr* ignore_parens();
  [[nodiscard]] const PAstExpr* ignore_parens() const { return const_cast<PAstExpr*>(this)->ignore_parens(); }

  [[nodiscard]] virtual PValueCategory get_value_category() const { return P_VC_RVALUE; }
  [[nodiscard]] bool is_lvalue() const { return get_value_category() == P_VC_LVALUE; }
  [[nodiscard]] bool is_rvalue() const { return get_value_category() == P_VC_RVALUE; }

  [[nodiscard]] virtual PType* get_type(PContext& p_ctx) const = 0;

protected:
  PAstExpr(PStmtKind p_kind, PSourceRange p_src_range)
    : PAst(p_kind, p_src_range)
  {
  }
};

/// \brief A bool literal (e.g. `true` or `false`).
class PAstBoolLiteral : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_BOOL_LITERAL;

  bool value;

  explicit PAstBoolLiteral(bool p_value, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , value(p_value)
  {
  }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return p_ctx.get_bool_ty(); }
};

/// \brief An integer literal (e.g. `42`).
class PAstIntLiteral : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_INT_LITERAL;

  PType* type;
  uintmax_t value;

  PAstIntLiteral(uintmax_t p_value, PType* p_type, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , value(p_value)
  {
    type = p_type;
  }

  [[nodiscard]] PType* get_type(PContext&) const override { return type; }
};

/// \brief A float literal (e.g. `3.14`).
class PAstFloatLiteral : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_FLOAT_LITERAL;

  PType* type;
  double value;

  PAstFloatLiteral(double p_value, PType* p_type, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , value(p_value)
  {
    type = p_type;
  }

  [[nodiscard]] PType* get_type(PContext&) const override { return type; }
};

/// \brief A parenthesized expression (e.g. `(sub_expr)`).
class PAstParenExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_PAREN_EXPR;

  PAstExpr* sub_expr;

  PAstParenExpr(PAstExpr* p_sub_expr, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , sub_expr(p_sub_expr)
  {
    assert(p_sub_expr != nullptr);
  }

  [[nodiscard]] PValueCategory get_value_category() const override { return sub_expr->get_value_category(); }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return sub_expr->get_type(p_ctx); }
};

/// \brief A declaration reference (e.g. `x` where x is a variable).
class PAstDeclRefExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_DECL_REF_EXPR;

  PDecl* decl;

  PAstDeclRefExpr(PDecl* p_decl, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , decl(p_decl)
  {
    assert(p_decl != nullptr);
  }

  [[nodiscard]] PValueCategory get_value_category() const override { return P_VC_LVALUE; }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return decl->get_type(); }
};

/// \brief Supported unary operators.
enum PAstUnaryOp
{
#define UNARY_OPERATOR(p_kind, p_tok) p_kind,
#include "operator_kinds.def"
};

/// Gets the spelling of the given unary operator. The returned string is
/// statically allocated and therefore does not need to be freed.
const char*
p_get_spelling(PAstUnaryOp p_opcode);

/// \brief An unary expression (e.g. `-x`).
class PAstUnaryExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_UNARY_EXPR;

  PType* type;
  PAstExpr* sub_expr;
  PAstUnaryOp opcode;

  PAstUnaryExpr(PAstExpr* p_sub_expr, PType* p_type, PAstUnaryOp p_opcode, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , type(p_type)
    , sub_expr(p_sub_expr)
    , opcode(p_opcode)
  {
  }

  [[nodiscard]] PValueCategory get_value_category() const override
  {
    switch (opcode) {
      case P_UNARY_ADDRESS_OF:
        return P_VC_LVALUE;
      default:
        return P_VC_RVALUE;
    }
  }

  [[nodiscard]] PType* get_type(PContext&) const override { return type; }
};

/// \brief Supported binary operators.
enum PAstBinaryOp
{
#define BINARY_OPERATOR(p_kind, p_tok, p_precedence) p_kind,
#include "operator_kinds.def"
};

bool
p_binop_is_assignment(PAstBinaryOp p_binop);

/// Gets the spelling of the given binary operator. The returned string is
/// statically allocated and therefore does not need to be freed.
const char*
p_get_spelling(PAstBinaryOp p_opcode);

/// \brief A binary expression (e.g. `a + b`).
class PAstBinaryExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_BINARY_EXPR;

  PType* type;
  PAstExpr* lhs;
  PAstExpr* rhs;
  PAstBinaryOp opcode;

  PAstBinaryExpr(PAstExpr* p_lhs, PAstExpr* p_rhs, PType* p_type, PAstBinaryOp p_opcode, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , type(p_type)
    , lhs(p_lhs)
    , rhs(p_rhs)
    , opcode(p_opcode)
  {
  }

  [[nodiscard]] PValueCategory get_value_category() const override
  {
    if (p_binop_is_assignment(opcode)) {
      return P_VC_LVALUE;
    }

    return P_VC_RVALUE;
  }

  [[nodiscard]] PType* get_type(PContext&) const override { return type; }
};

class PStructFieldDecl;

/// \brief A member access expression (e.g. `base_expr.member`).
class PAstMemberExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_MEMBER_EXPR;

  PAstExpr* base_expr;
  PStructFieldDecl* member;

  PAstMemberExpr(PAstExpr* p_base_expr, PStructFieldDecl* p_member, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , base_expr(p_base_expr)
    , member(p_member)
  {
  }

  [[nodiscard]] PValueCategory get_value_category() const override { return P_VC_LVALUE; }

  [[nodiscard]] PType* get_type(PContext&) const override { return member->get_type(); }
};

/// \brief A function call.
class PAstCallExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_CALL_EXPR;

  PAstExpr* callee;
  PArrayView<PAstExpr*> args;

  PAstCallExpr(PAstExpr* p_callee, PArrayView<PAstExpr*> p_args, PSourceRange p_src_range = {})
    : PAstExpr(STMT_KIND, p_src_range)
    , callee(p_callee)
    , args(p_args)
  {
  }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override
  {
    auto* callee_ty = callee->get_type(p_ctx);
    if (!callee_ty->is_function_ty())
      return nullptr;

    return callee_ty->as<PFunctionType>()->get_ret_ty();
  }
};

/// \brief Supported cast kinds.
enum PAstCastKind
{
#define CAST_OPERATOR(p_kind) p_kind,
#include "operator_kinds.def"
};

/// \brief A cast expression (e.g. `sub_expr as i32`).
class PAstCastExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_CAST_EXPR;

  PType* target_ty;
  PAstExpr* sub_expr;
  PAstCastKind cast_kind;

  PAstCastExpr(PAstExpr* p_sub_expr, PType* p_target_type, PAstCastKind p_kind, PSourceRange p_src_range)
    : PAstExpr(STMT_KIND, p_src_range)
    , target_ty(p_target_type)
    , sub_expr(p_sub_expr)
    , cast_kind(p_kind)
  {
    assert(p_sub_expr != nullptr && p_target_type != nullptr);
  }

  [[nodiscard]] PValueCategory get_value_category() const override { return sub_expr->get_value_category(); }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return target_ty; }
};

/// \brief A field in a <em>struct expression</em>.
///
/// \note PAstStructFieldExpr does not inherit from PAstExpr because it is
///       not a valid expression alone. It is only a part of a bigger expression: PAstStructExpr.
class PAstStructFieldExpr
{
public:
  PAstStructFieldExpr(PStructFieldDecl* p_field_decl, PAstExpr* p_expr, bool m_shorthand = false);
  ~PAstStructFieldExpr() = delete;

  [[nodiscard]] PStructFieldDecl* get_field_decl() const { return m_field_decl; }

  [[nodiscard]] PLocalizedIdentifierInfo get_localized_name() const { return m_field_decl->get_localized_name(); }
  [[nodiscard]] PIdentifierInfo* get_name() const { return m_field_decl->get_name(); }
  [[nodiscard]] PSourceRange get_name_range() const { return m_field_decl->get_name_range(); }

  [[nodiscard]] PAstExpr* get_expr() const { return m_expr; }
  [[nodiscard]] PSourceRange get_expr_range() const { return m_expr->get_source_range(); }

  /// Returns `true` if it is a shorthand field expression, that is a field
  /// expression with only a field name as the field `a` in the following example:
  /// ```
  /// MyStruct {
  ///    a, // equivalent to 'a: a'
  ///    b: true
  /// }
  /// ```
  [[nodiscard]] bool is_shorthand() const { return m_is_shorthand; }

private:
  PStructFieldDecl* m_field_decl;
  PAstExpr* m_expr;
  bool m_is_shorthand;
};

/// \brief A <em>struct expression</em> creates a struct value.
///
/// Example:
/// \code{.rs}
/// MyStruct { foo: 5, bar: a_function() / 2, baz };
/// \endcode
class PAstStructExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_STRUCT_EXPR;

  PAstStructExpr(PStructDecl* p_struct_decl,
                 PArrayView<PAstStructFieldExpr*> p_fields,
                 PSourceRange p_src_range = {});
  ~PAstStructExpr() = delete;

  [[nodiscard]] PStructDecl* get_struct_decl() const { return m_struct_decl; }

  [[nodiscard]] size_t get_field_count() const { return m_fields.size(); }
  [[nodiscard]] PArrayView<PAstStructFieldExpr*> get_fields() const { return m_fields; }

  [[nodiscard]] PType* get_type(PContext&) const override { return m_struct_decl->get_type(); }

private:
  PStructDecl* m_struct_decl;
  PArrayView<PAstStructFieldExpr*> m_fields;
};

/// \brief An implicit lvalue -> rvalue cast generated by the semantic analyzer.
class PAstL2RValueExpr : public PAstExpr
{
public:
  static constexpr auto STMT_KIND = P_SK_L2RVALUE_EXPR;

  PAstExpr* sub_expr;

  explicit PAstL2RValueExpr(PAstExpr* p_sub_expr)
    : PAstExpr(STMT_KIND, p_sub_expr->get_source_range())
    , sub_expr(p_sub_expr)
  {
  }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return sub_expr->get_type(p_ctx); }
};

#endif // PEONY_AST_EXPR_HXX
