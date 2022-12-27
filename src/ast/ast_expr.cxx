#include "ast_expr.hxx"

PAstExpr*
PAstExpr::ignore_parens()
{
  PAstExpr* node = this;
  while (node != nullptr && node->get_kind() == P_AST_NODE_PAREN_EXPR) {
    node = node->as<PAstParenExpr>()->sub_expr;
  }
  return node;
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
