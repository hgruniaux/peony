#ifndef PEONY_AST_EXPR_HXX
#define PEONY_AST_EXPR_HXX

#include "ast_stmt.hxx"
#include "ast_decl.hxx"

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
  bool value;

  explicit PAstBoolLiteral(bool p_value, PSourceRange p_src_range = {})
    : PAstExpr(P_AST_NODE_BOOL_LITERAL, p_src_range)
    , value(p_value)
  {
  }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return p_ctx.get_bool_ty(); }
};

/// \brief An integer literal (e.g. `42`).
class PAstIntLiteral : public PAstExpr
{
public:
  PType* type;
  uintmax_t value;

  PAstIntLiteral(uintmax_t p_value, PType* p_type, PSourceRange p_src_range = {})
    : PAstExpr(P_AST_NODE_INT_LITERAL, p_src_range)
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
  PType* type;
  double value;

  PAstFloatLiteral(double p_value, PType* p_type, PSourceRange p_src_range = {})
    : PAstExpr(P_AST_NODE_FLOAT_LITERAL, p_src_range)
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
  PAstExpr* sub_expr;

  PAstParenExpr(PAstExpr* p_sub_expr, PSourceRange p_src_range = {})
    : PAstExpr(P_AST_NODE_PAREN_EXPR, p_src_range)
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
  PDecl* decl;

  PAstDeclRefExpr(PDecl* p_decl, PSourceRange p_src_range = {})
    : PAstExpr(P_AST_NODE_DECL_REF_EXPR, p_src_range)
    , decl(p_decl)
  {
    assert(p_decl != nullptr);
  }

  [[nodiscard]] PValueCategory get_value_category() const override { return P_VC_LVALUE; }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return decl->type; }
};

/// \brief Supported unary operators.
enum PAstUnaryOp
{
#define UNARY_OPERATOR(p_kind, p_tok) p_kind,
#include "operator_kinds.def"
};

/// Gets the spelling of the given unary operator. The returned string is
/// statically allocated and therefore does not need to be freed.
const char* p_get_spelling(PAstUnaryOp p_opcode);

/// \brief An unary expression (e.g. `-x`).
class PAstUnaryExpr : public PAstExpr
{
public:
  PType* type;
  PAstExpr* sub_expr;
  PAstUnaryOp opcode;

  PAstUnaryExpr(PAstExpr* p_sub_expr, PType* p_type, PAstUnaryOp p_opcode, PSourceRange p_src_range = {})
    : PAstExpr(P_AST_NODE_UNARY_EXPR, p_src_range)
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
const char* p_get_spelling(PAstBinaryOp p_opcode);

/// \brief A binary expression (e.g. `a + b`).
class PAstBinaryExpr : public PAstExpr
{
public:
  PType* type;
  PAstExpr* lhs;
  PAstExpr* rhs;
  PAstBinaryOp opcode;

  PAstBinaryExpr(PAstExpr* p_lhs, PAstExpr* p_rhs, PType* p_type, PAstBinaryOp p_opcode, PSourceRange p_src_range = {})
    : PAstExpr(P_AST_NODE_BINARY_EXPR, p_src_range)
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
  PAstExpr* base_expr;
  PStructFieldDecl* member;

  [[nodiscard]] PType* get_type(PContext&) const override { return member->type; }
};

/// \brief A function call.
class PAstCallExpr : public PAstExpr
{
public:
  PAstExpr* callee;
  PAstExpr** args;
  size_t arg_count;

  PAstCallExpr(PAstExpr* p_callee, PAstExpr** p_args, size_t p_arg_count, PSourceRange p_src_range = {})
    : PAstExpr(P_AST_NODE_CALL_EXPR, p_src_range)
    , callee(p_callee)
    , args(p_args)
    , arg_count(p_arg_count)
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
  PType* target_ty;
  PAstExpr* sub_expr;
  PAstCastKind cast_kind;

  PAstCastExpr(PAstExpr* p_sub_expr, PType* p_target_type, PAstCastKind p_kind, PSourceRange p_src_range)
    : PAstExpr(P_AST_NODE_CAST_EXPR, p_src_range)
    , target_ty(p_target_type)
    , sub_expr(p_sub_expr)
    , cast_kind(p_kind)
  {
    assert(p_sub_expr != nullptr && p_target_type != nullptr);
  }

  [[nodiscard]] PValueCategory get_value_category() const override { return sub_expr->get_value_category(); }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return target_ty; }
};

/// \brief An implicit lvalue -> rvalue cast generated by the semantic analyzer.
class PAstL2RValueExpr : public PAstExpr
{
public:
  PAstExpr* sub_expr;

  explicit PAstL2RValueExpr(PAstExpr* p_sub_expr)
    : PAstExpr(P_AST_NODE_L2RVALUE_EXPR, p_sub_expr->get_source_range())
    , sub_expr(p_sub_expr)
  {
  }

  [[nodiscard]] PType* get_type(PContext& p_ctx) const override { return sub_expr->get_type(p_ctx); }
};

#endif // PEONY_AST_EXPR_HXX
