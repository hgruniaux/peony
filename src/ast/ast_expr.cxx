#include "ast_expr.hxx"

#include "../interpreter/interpreter.hxx"

PAstExpr*
PAstExpr::ignore_parens()
{
  PAstExpr* node = this;
  while (node != nullptr && node->get_kind() == P_SK_PAREN_EXPR) {
    node = node->as<PAstParenExpr>()->sub_expr;
  }
  return node;
}

const PAstExpr*
PAstExpr::ignore_parens() const
{
  return const_cast<PAstExpr*>(this)->ignore_parens();
}

PAstExpr*
PAstExpr::ignore_parens_and_casts()
{
  PAstExpr* node = this;
  while (node != nullptr && (node->get_kind() == P_SK_PAREN_EXPR || node->get_kind() == P_SK_CAST_EXPR)) {
    node = node->as<PAstParenExpr>()->sub_expr;
  }
  return node;
}

const PAstExpr*
PAstExpr::ignore_parens_and_casts() const
{
  return const_cast<PAstExpr*>(this)->ignore_parens_and_casts();
}

std::optional<bool>
PAstExpr::eval_as_bool(PContext& p_ctx) const
{
  PInterpreter inter(p_ctx);
  return inter.eval_as_bool(this);
}

bool
p_binop_is_assignment(PAstBinaryOp p_binop)
{
  switch (p_binop) {
    case P_BINARY_ASSIGN:
    case P_BINARY_ASSIGN_MUL:
    case P_BINARY_ASSIGN_DIV:
    case P_BINARY_ASSIGN_MOD:
    case P_BINARY_ASSIGN_ADD:
    case P_BINARY_ASSIGN_SUB:
    case P_BINARY_ASSIGN_SHL:
    case P_BINARY_ASSIGN_SHR:
    case P_BINARY_ASSIGN_BIT_AND:
    case P_BINARY_ASSIGN_BIT_XOR:
    case P_BINARY_ASSIGN_BIT_OR:
      return true;
    default:
      return false;
  }
}

const char*
p_get_spelling(PAstUnaryOp p_opcode)
{
  switch (p_opcode) {
#define UNARY_OPERATOR(p_kind, p_tok)                                                                                  \
  case p_kind:                                                                                                         \
    return token_kind_get_spelling(p_tok);
#include "operator_kinds.def"
    default:
      assert(false && "not a unary operator");
      return nullptr;
  }
}

const char*
p_get_spelling(PAstBinaryOp p_opcode)
{
  switch (p_opcode) {
#define BINARY_OPERATOR(p_kind, p_tok, p_precedence)                                                                   \
  case p_kind:                                                                                                         \
    return token_kind_get_spelling(p_tok);
#include "operator_kinds.def"
    default:
      assert(false && "not a binary operator");
      return nullptr;
  }
}

PAstStructFieldExpr::PAstStructFieldExpr(PStructFieldDecl* p_field_decl, PAstExpr* p_expr, bool m_shorthand)
  : m_field_decl(p_field_decl)
  , m_expr(p_expr)
  , m_is_shorthand(m_shorthand)
{
}

PAstStructExpr::PAstStructExpr(PStructDecl* p_struct_decl,
                               PArrayView<PAstStructFieldExpr*> p_fields,
                               PSourceRange p_src_range)
  : PAstExpr(STMT_KIND, p_src_range)
  , m_struct_decl(p_struct_decl)
  , m_fields(p_fields)
{
}
