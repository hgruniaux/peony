#include "ast.h"

#include "utils/bump_allocator.h"

#include <assert.h>

bool
p_binop_is_assignment(PAstBinaryOp p_binop)
{
  switch (p_binop) {
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

PAst*
p_create_node_impl(size_t p_size, PAstKind p_kind)
{
  PAst* node = p_bump_alloc(&p_global_bump_allocator, p_size, P_ALIGNOF(PAst));
  assert(node != NULL);
  P_AST_GET_KIND(node) = p_kind;
  return node;
}

PDecl*
p_create_decl_impl(size_t p_size, PDeclKind p_kind)
{
  PDecl* decl = p_bump_alloc(&p_global_bump_allocator, p_size, P_ALIGNOF(PDecl));
  assert(decl != NULL);
  decl->common.kind = p_kind;
  decl->common.name = NULL;
  decl->common.type = NULL;
  decl->common._llvm_address = NULL;
  decl->common.used = false;
  return decl;
}
