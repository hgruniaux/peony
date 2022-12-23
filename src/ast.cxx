#include "ast.hxx"

#include "utils/bump_allocator.hxx"

#include <cassert>

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
p_binop_get_spelling(PAstBinaryOp p_binop)
{
  switch (p_binop) {
#define BINARY_OPERATOR(p_kind, p_tok, p_precedence)                                                                   \
  case p_kind:                                                                                                         \
    return token_kind_get_spelling(p_tok);
#include "operator_kinds.def"
    default:
      HEDLEY_UNREACHABLE_RETURN(nullptr);
  }
}

#define CAST(p_node, p_type) ((p_type*)(p_node))

PType*
p_ast_get_type(PAst* p_ast)
{
  assert(p_ast != nullptr);

#define DUMMY_TYPE p_type_get_i32()
  switch (P_AST_GET_KIND(p_ast)) {
    case P_AST_NODE_INT_LITERAL:
    case P_AST_NODE_FLOAT_LITERAL:
    case P_AST_NODE_UNARY_EXPR:
    case P_AST_NODE_BINARY_EXPR:
    case P_AST_NODE_CAST_EXPR:
      return CAST(p_ast, PAstIntLiteral)->type;

    case P_AST_NODE_BOOL_LITERAL:
      return p_type_get_bool();
    case P_AST_NODE_PAREN_EXPR:
      return p_ast_get_type(CAST(p_ast, PAstParenExpr)->sub_expr);
    case P_AST_NODE_LVALUE_TO_RVALUE_EXPR:
      return p_ast_get_type(CAST(p_ast, PAstLValueToRValueExpr)->sub_expr);
    case P_AST_NODE_DECL_REF_EXPR: {
      PDecl* decl = CAST(p_ast, PAstDeclRefExpr)->decl;
      if (decl == nullptr)
        return DUMMY_TYPE;

      return P_DECL_GET_TYPE(decl);
    }
    case P_AST_NODE_CALL_EXPR: {
      PType* callee_type = p_ast_get_type(CAST(p_ast, PAstCallExpr)->callee);
      if (!p_type_is_function(callee_type))
        return DUMMY_TYPE;

      return CAST(callee_type, PFunctionType)->ret_type;
    }
    case P_AST_NODE_MEMBER_EXPR: {
      return P_DECL_GET_TYPE(CAST(p_ast, PAstMemberExpr)->member);
    }

    default: // This node has no concept of type.
      assert(false && "p_node is not an expression node");
      return nullptr;
  }
#undef DUMMY_TYPE
}

PAst*
p_ast_ignore_parens(PAst* p_ast)
{
  while (true) {
    switch (P_AST_GET_KIND(p_ast)) {
      case P_AST_NODE_PAREN_EXPR:
        p_ast = ((PAstParenExpr*)p_ast)->sub_expr;
        continue;
      default:
        return p_ast;
    }
  }
}

PAst*
p_create_node_impl(size_t p_size, PAstKind p_kind)
{
  PAst* node = static_cast<PAst*>(p_global_bump_allocator.alloc(p_size, alignof(PAst)));
  assert(node != nullptr);
  P_AST_GET_KIND(node) = p_kind;
  return node;
}

PDecl*
p_create_decl_impl(size_t p_size, PDeclKind p_kind)
{
  auto* decl = static_cast<PDecl*>(p_global_bump_allocator.alloc(p_size, alignof(PDecl)));
  assert(decl != nullptr);
  decl->common.kind = p_kind;
  decl->common.name = nullptr;
  decl->common.type = nullptr;
  decl->common._llvm_address = nullptr;
  decl->common.used = false;
  return decl;
}
