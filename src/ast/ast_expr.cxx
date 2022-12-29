#include "ast_expr.hxx"

PAstExpr*
PAstExpr::ignore_parens()
{
  PAstExpr* node = this;
  while (node != nullptr && node->get_kind() == P_SK_PAREN_EXPR) {
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
