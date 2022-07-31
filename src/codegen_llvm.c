#include "codegen_llvm.h"

#include "hedley.h"
#include "identifier_table.h"

#include <assert.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>

static LLVMTypeRef
cg_to_llvm_type(PType* p_type)
{
  assert(p_type != NULL);

  if (p_type->common._llvm_cached_type != NULL)
    return p_type->common._llvm_cached_type;

  LLVMTypeRef type;
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_VOID:
      type = LLVMVoidType();
      break;
    case P_TYPE_BOOL:
      type = LLVMInt1Type();
      break;
    case P_TYPE_I8:
    case P_TYPE_U8:
      type = LLVMInt8Type();
      break;
    case P_TYPE_I16:
    case P_TYPE_U16:
      type = LLVMInt16Type();
      break;
    case P_TYPE_I32:
    case P_TYPE_U32:
      type = LLVMInt32Type();
      break;
    case P_TYPE_I64:
    case P_TYPE_U64:
      type = LLVMInt64Type();
      break;
    case P_TYPE_F32:
      type = LLVMFloatType();
      break;
    case P_TYPE_F64:
      type = LLVMDoubleType();
      break;
    case P_TYPE_FUNCTION: {
      PFunctionType* func_type = (PFunctionType*)p_type;
      LLVMTypeRef* args = malloc(sizeof(LLVMTypeRef) * func_type->arg_count);
      assert(args != NULL);

      for (int i = 0; i < func_type->arg_count; ++i) {
        args[i] = cg_to_llvm_type(func_type->args[i]);
      }

      type = LLVMFunctionType(cg_to_llvm_type(func_type->ret_type), args, func_type->arg_count, false);
      free(args);
      break;
    }
    case P_TYPE_UNDEF:
    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }

  p_type->common._llvm_cached_type = type;
  return type;
}

static LLVMValueRef
cg_visit(struct PCodegenLLVM* p_cg, PAst* p_node);

/* Inserts a dummy block just after the current insertion block
 * and position the builder at end of it. Used to ensure that
 * each terminal instruction is at end of a block (when generating
 * code for `break;` or `return;` statements for example).
 */
static void
cg_insert_dummy_block(struct PCodegenLLVM* p_cg)
{
  const LLVMBasicBlockRef bb = LLVMAppendBasicBlock(p_cg->current_function, "");
  LLVMPositionBuilderAtEnd(p_cg->builder, bb);
}

/* Inserts an alloca instruction in the entry block of the current function. */
static LLVMValueRef
cg_insert_alloc_in_entry_bb(struct PCodegenLLVM* p_cg, LLVMTypeRef p_type)
{
  /* Save the position of the builder to restore it after. */
  const LLVMBasicBlockRef current_block = LLVMGetInsertBlock(p_cg->builder);

  const LLVMBasicBlockRef entry_block = LLVMGetEntryBasicBlock(p_cg->current_function);
  const LLVMValueRef first_inst = LLVMGetFirstInstruction(entry_block);
  LLVMPositionBuilder(p_cg->builder, entry_block, first_inst);
  const LLVMValueRef address = LLVMBuildAlloca(p_cg->builder, p_type, "");

  LLVMPositionBuilderAtEnd(p_cg->builder, current_block);
  return address;
}

static void
cg_finish_function_codegen(struct PCodegenLLVM* p_cg)
{
  /* Correct all empty blocks that may have been generated. */
  LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(p_cg->current_function);
  while (block != NULL) {
    if (LLVMGetBasicBlockTerminator(block) == NULL) {
      LLVMPositionBuilderAtEnd(p_cg->builder, block);
      LLVMBuildUnreachable(p_cg->builder);
    }

    block = LLVMGetNextBasicBlock(block);
  }
}

static void
cg_function_decl(struct PCodegenLLVM* p_cg, PDeclFunction* p_decl)
{
  assert(P_DECL_GET_KIND(p_decl) == P_DECL_FUNCTION);

  const char* name = P_DECL_GET_NAME(p_decl)->spelling;
  LLVMTypeRef type = cg_to_llvm_type(P_DECL_GET_TYPE(p_decl));
  LLVMValueRef func = LLVMAddFunction(p_cg->module, name, type);
  p_decl->common._llvm_address = func;

  if (p_decl->body != NULL) {
    p_cg->current_function = func;
    LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(func, "");
    LLVMPositionBuilderAtEnd(p_cg->builder, entry_bb);

    for (int i = 0; i < p_decl->param_count; ++i) {
      LLVMTypeRef param_ty = cg_to_llvm_type(P_DECL_GET_TYPE(p_decl->params[i]));
      LLVMValueRef param_addr = LLVMBuildAlloca(p_cg->builder, param_ty, "");
      LLVMBuildStore(p_cg->builder, LLVMGetParam(p_cg->current_function, i), param_addr);
      p_decl->params[i]->common._llvm_address = param_addr;
    }

    cg_visit(p_cg, p_decl->body);
    cg_finish_function_codegen(p_cg);
  }
}

static void
cg_var_decl(struct PCodegenLLVM* p_cg, PDeclVar* p_decl)
{
  assert(P_DECL_GET_KIND(p_decl) == P_DECL_VAR);

  LLVMTypeRef type = cg_to_llvm_type(P_DECL_GET_TYPE(p_decl));
  LLVMValueRef address = cg_insert_alloc_in_entry_bb(p_cg, type);
  p_decl->common._llvm_address = address;

  if (p_decl->init_expr != NULL) {
    const LLVMValueRef init_value = cg_visit(p_cg, p_decl->init_expr);
    LLVMBuildStore(p_cg->builder, init_value, address);
  }
}

static void
cg_visit_decl(struct PCodegenLLVM* p_cg, PDecl* p_decl)
{
  assert(p_decl != NULL);

  switch (P_DECL_GET_KIND(p_decl)) {
    case P_DECL_FUNCTION:
      cg_function_decl(p_cg, (PDeclFunction*)p_decl);
      break;
    case P_DECL_VAR:
      cg_var_decl(p_cg, (PDeclVar*)p_decl);
      break;
    default:
      HEDLEY_UNREACHABLE();
  }
}

static LLVMValueRef
cg_compound_stmt(struct PCodegenLLVM* p_cg, PAstCompoundStmt* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_COMPOUND_STMT);

  for (int i = 0; i < p_node->stmt_count; ++i) {
    PAst* stmt = p_node->stmts[i];
    cg_visit(p_cg, stmt);
  }

  return NULL;
}

static LLVMValueRef
cg_let_stmt(struct PCodegenLLVM* p_cg, PAstLetStmt* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_LET_STMT);

  cg_visit_decl(p_cg, p_node->var_decl);
  return NULL;
}

static LLVMValueRef
cg_return_stmt(struct PCodegenLLVM* p_cg, PAstReturnStmt* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_RETURN_STMT);

  if (p_node->ret_expr != NULL) {
    const LLVMValueRef ret_value = cg_visit(p_cg, p_node->ret_expr);
    LLVMBuildRet(p_cg->builder, ret_value);
  } else {
    LLVMBuildRetVoid(p_cg->builder);
  }

  cg_insert_dummy_block(p_cg);

  return NULL;
}

static LLVMValueRef
cg_while_stmt(struct PCodegenLLVM* p_cg, PAstWhileStmt* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_WHILE_STMT);

  LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(p_cg->current_function, "");
  LLVMBasicBlockRef body_bb = LLVMAppendBasicBlock(p_cg->current_function, "");
  LLVMBasicBlockRef cont_bb = LLVMAppendBasicBlock(p_cg->current_function, "");

  LLVMBuildBr(p_cg->builder, entry_bb);

  /* Entry block: */
  LLVMPositionBuilderAtEnd(p_cg->builder, entry_bb);
  const LLVMValueRef cond = cg_visit(p_cg, p_node->cond_expr);
  LLVMBuildCondBr(p_cg->builder, cond, body_bb, cont_bb);

  /* Body block: */
  LLVMPositionBuilderAtEnd(p_cg->builder, body_bb);
  cg_visit(p_cg, p_node->body_stmt);
  LLVMBuildBr(p_cg->builder, entry_bb);

  /* Cont block: */
  LLVMPositionBuilderAtEnd(p_cg->builder, cont_bb);
  return NULL;
}

static LLVMValueRef
cg_if_stmt(struct PCodegenLLVM* p_cg, PAstIfStmt* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_IF_STMT);

  // TODO: Support if statements as expressions.
  // Example: let x = if (true) { 4 } else { 3 }.
  // Maybe use PHI nodes in that case.

  LLVMBasicBlockRef then_bb = LLVMAppendBasicBlock(p_cg->current_function, "");
  LLVMBasicBlockRef cont_bb = LLVMAppendBasicBlock(p_cg->current_function, "");
  LLVMBasicBlockRef else_bb = NULL;
  if (p_node->else_stmt != NULL) {
    else_bb = LLVMAppendBasicBlock(p_cg->current_function, "");
  }

  const LLVMValueRef cond = cg_visit(p_cg, p_node->cond_expr);
  LLVMBuildCondBr(p_cg->builder, cond, then_bb, else_bb != NULL ? else_bb : cont_bb);

  /* Then block: */
  LLVMPositionBuilderAtEnd(p_cg->builder, then_bb);
  cg_visit(p_cg, p_node->then_stmt);
  LLVMBuildBr(p_cg->builder, cont_bb);

  if (p_node->else_stmt != NULL) {
    /* Else block: */
    LLVMPositionBuilderAtEnd(p_cg->builder, else_bb);
    cg_visit(p_cg, p_node->else_stmt);
    LLVMBuildBr(p_cg->builder, cont_bb);
  }

  /* Cont block: */
  LLVMPositionBuilderAtEnd(p_cg->builder, cont_bb);
  return NULL;
}

static LLVMValueRef
cg_bool_literal(struct PCodegenLLVM* p_cg, PAstBoolLiteral* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_BOOL_LITERAL);

  if (p_node->value) {
    return LLVMConstInt(LLVMInt1Type(), 1, 0);
  } else {
    return LLVMConstInt(LLVMInt1Type(), 0, 0);
  }
}

static LLVMValueRef
cg_int_literal(struct PCodegenLLVM* p_cg, PAstIntLiteral* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_INT_LITERAL);

  LLVMTypeRef type = cg_to_llvm_type(P_AST_GET_TYPE(p_node));
  return LLVMConstInt(type, p_node->value, 0);
}

static LLVMValueRef
cg_float_literal(struct PCodegenLLVM* p_cg, PAstFloatLiteral* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_FLOAT_LITERAL);

  LLVMTypeRef type = cg_to_llvm_type(P_AST_GET_TYPE(p_node));
  return LLVMConstReal(type, p_node->value);
}

static LLVMValueRef
cg_paren_expr(struct PCodegenLLVM* p_cg, PAstParenExpr* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_PAREN_EXPR);

  return cg_visit(p_cg, p_node->sub_expr);
}

static LLVMValueRef
cg_decl_ref_expr(struct PCodegenLLVM* p_cg, PAstDeclRefExpr* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_DECL_REF_EXPR);

  LLVMValueRef addr = p_node->decl->common._llvm_address;
  if (p_cg->is_in_lhs) {
    return addr;
  } else {
    return LLVMBuildLoad(p_cg->builder, addr, "");
  }
}

static LLVMValueRef
cg_unary_expr(struct PCodegenLLVM* p_cg, PAstUnaryExpr* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_UNARY_EXPR);

  LLVMValueRef sub_expr = cg_visit(p_cg, p_node->sub_expr);
  switch (p_node->opcode) {
    case P_UNARY_NEG:
      if (p_type_is_signed(P_AST_GET_TYPE(p_node)))
        return LLVMBuildNeg(p_cg->builder, sub_expr, "");
      else if (p_type_is_float(P_AST_GET_TYPE(p_node)))
        return LLVMBuildFNeg(p_cg->builder, sub_expr, "");
      else
        HEDLEY_UNREACHABLE_RETURN(NULL);
    case P_UNARY_NOT:
      return LLVMBuildNot(p_cg->builder, sub_expr, "");
    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }
}

/* Implements all trivial binary opcodes. Compound assignments (e.g. +=)
 * or logical AND and OR (lazy operators) are not implemented by this function.
 */
static LLVMValueRef
cg_binary_expr_impl(struct PCodegenLLVM* p_cg, PType* p_type, PAstBinaryOp p_op, LLVMValueRef p_lhs, LLVMValueRef p_rhs)
{
  switch (p_op) {
#define DISPATCH(p_signed_fn, p_unsigned_fn, p_float_fn)                                                               \
  if (p_type_is_signed(p_type)) {                                                                                      \
    return p_signed_fn(p_cg->builder, p_lhs, p_rhs, "");                                                               \
  } else if (p_type_is_unsigned(p_type)) {                                                                             \
    return p_unsigned_fn(p_cg->builder, p_lhs, p_rhs, "");                                                             \
  } else if (p_type_is_float(p_type)) {                                                                                \
    return p_float_fn(p_cg->builder, p_lhs, p_rhs, "");                                                                \
  } else {                                                                                                             \
    HEDLEY_UNREACHABLE_RETURN(NULL);                                                                                   \
  }

    case P_BINARY_ADD:
      DISPATCH(LLVMBuildNSWAdd, LLVMBuildAdd, LLVMBuildFAdd)
    case P_BINARY_SUB:
      DISPATCH(LLVMBuildNSWSub, LLVMBuildSub, LLVMBuildFSub)
    case P_BINARY_MUL:
      DISPATCH(LLVMBuildNSWMul, LLVMBuildMul, LLVMBuildFMul)
    case P_BINARY_DIV:
      DISPATCH(LLVMBuildSDiv, LLVMBuildUDiv, LLVMBuildFDiv)
    case P_BINARY_MOD:
      DISPATCH(LLVMBuildSRem, LLVMBuildURem, LLVMBuildFRem)

#define DISPATCH_ONLY_INT(p_signed_fn, p_unsigned_fn)                                                                  \
  if (p_type_is_signed(p_type)) {                                                                                      \
    return p_signed_fn(p_cg->builder, p_lhs, p_rhs, "");                                                               \
  } else if (p_type_is_unsigned(p_type)) {                                                                             \
    return p_unsigned_fn(p_cg->builder, p_lhs, p_rhs, "");                                                             \
  } else {                                                                                                             \
    HEDLEY_UNREACHABLE_RETURN(NULL);                                                                                   \
  }

    case P_BINARY_SHL:
      DISPATCH_ONLY_INT(LLVMBuildShl, LLVMBuildShl)
    case P_BINARY_SHR:
      DISPATCH_ONLY_INT(LLVMBuildAShr, LLVMBuildLShr)
    case P_BINARY_BIT_AND:
      DISPATCH_ONLY_INT(LLVMBuildAnd, LLVMBuildAnd)
    case P_BINARY_BIT_XOR:
      DISPATCH_ONLY_INT(LLVMBuildXor, LLVMBuildXor)
    case P_BINARY_BIT_OR:
      DISPATCH_ONLY_INT(LLVMBuildOr, LLVMBuildOr)

#define DISPATCH_COMP(p_signed_op, p_unsigned_op, p_float_op)                                                          \
  if (p_type_is_signed(p_type)) {                                                                                      \
    return LLVMBuildICmp(p_cg->builder, p_signed_op, p_lhs, p_rhs, "");                                                \
  } else if (p_type_is_unsigned(p_type)) {                                                                             \
    return LLVMBuildICmp(p_cg->builder, p_unsigned_op, p_lhs, p_rhs, "");                                              \
  } else if (p_type_is_float(p_type)) {                                                                                \
    return LLVMBuildFCmp(p_cg->builder, p_float_op, p_lhs, p_rhs, "");                                                 \
  } else {                                                                                                             \
    HEDLEY_UNREACHABLE_RETURN(NULL);                                                                                   \
  }

    case P_BINARY_LT:
      DISPATCH_COMP(LLVMIntSLT, LLVMIntULT, LLVMRealOLT)
    case P_BINARY_LE:
      DISPATCH_COMP(LLVMIntSLE, LLVMIntULE, LLVMRealOLE)
    case P_BINARY_GT:
      DISPATCH_COMP(LLVMIntSGT, LLVMIntUGT, LLVMRealOGT)
    case P_BINARY_GE:
      DISPATCH_COMP(LLVMIntSGE, LLVMIntUGE, LLVMRealOGE)
    case P_BINARY_EQ:
      DISPATCH_COMP(LLVMIntEQ, LLVMIntEQ, LLVMRealOEQ)
    case P_BINARY_NE:
      DISPATCH_COMP(LLVMIntNE, LLVMIntNE, LLVMRealUNE)

#undef DISPATCH
#undef DISPATCH_ONLY_INT
#undef DISPATCH_COMP

    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }
}

/* Implements && or || binary operators. */
static LLVMValueRef
cg_log_and_impl(struct PCodegenLLVM* p_cg, PAst* p_lhs, PAst* p_rhs, bool is_and)
{
  LLVMBasicBlockRef rhs_bb = LLVMAppendBasicBlock(p_cg->current_function, "");
  LLVMBasicBlockRef cont_bb = LLVMAppendBasicBlock(p_cg->current_function, "");

  const LLVMValueRef lhs = cg_visit(p_cg, p_lhs);
  if (is_and) { /* If && */
    LLVMBuildCondBr(p_cg->builder, lhs, rhs_bb, cont_bb);
  } else { /* If || (just swap then_bb and else_bb) */
    LLVMBuildCondBr(p_cg->builder, lhs, cont_bb, rhs_bb);
  }

  LLVMValueRef incoming_values[2];
  LLVMBasicBlockRef incoming_blocks[2];

  /* Constant block (does not really exists because it is just a constant): */
  incoming_values[0] = LLVMConstInt(LLVMInt1Type(), !is_and, 0);
  incoming_blocks[0] = LLVMGetInsertBlock(p_cg->builder);

  /* RHS block: */
  LLVMPositionBuilderAtEnd(p_cg->builder, rhs_bb);
  incoming_values[1] = cg_visit(p_cg, p_rhs);
  incoming_blocks[1] = rhs_bb;
  LLVMBuildBr(p_cg->builder, cont_bb);

  /* Cont block: */
  LLVMPositionBuilderAtEnd(p_cg->builder, cont_bb);
  LLVMValueRef phi = LLVMBuildPhi(p_cg->builder, LLVMInt1Type(), "");
  LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);
  return phi;
}

static bool
is_assignment(PAstBinaryOp p_op)
{
  switch (p_op) {
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

static PAstBinaryOp
get_assignment_op(PAstBinaryOp p_op)
{
  switch (p_op) {
    case P_BINARY_ASSIGN_MUL:
      return P_BINARY_MUL;
    case P_BINARY_ASSIGN_DIV:
      return P_BINARY_DIV;
    case P_BINARY_ASSIGN_MOD:
      return P_BINARY_MOD;
    case P_BINARY_ASSIGN_ADD:
      return P_BINARY_ADD;
    case P_BINARY_ASSIGN_SUB:
      return P_BINARY_SUB;
    case P_BINARY_ASSIGN_SHL:
      return P_BINARY_SHL;
    case P_BINARY_ASSIGN_SHR:
      return P_BINARY_SHR;
    case P_BINARY_ASSIGN_BIT_AND:
      return P_BINARY_BIT_AND;
    case P_BINARY_ASSIGN_BIT_XOR:
      return P_BINARY_BIT_XOR;
    case P_BINARY_ASSIGN_BIT_OR:
      return P_BINARY_BIT_OR;
    default:
      HEDLEY_UNREACHABLE_RETURN(P_BINARY_ADD);
  }
}

/* Implements assignment ('=') and compound assignment (e.g. '+=') operators. */
static LLVMValueRef
cg_assignment_impl(struct PCodegenLLVM* p_cg, PType* p_type, PAstBinaryOp p_op, LLVMValueRef p_lhs, LLVMValueRef p_rhs)
{
  if (p_op == P_BINARY_ASSIGN) {
    return LLVMBuildStore(p_cg->builder, p_rhs, p_lhs);
  }

  p_op = get_assignment_op(p_op);

  const LLVMValueRef lhs_value = LLVMBuildLoad(p_cg->builder, p_lhs, "");
  const LLVMValueRef value = cg_binary_expr_impl(p_cg, p_type, p_op, lhs_value, p_rhs);
  return LLVMBuildStore(p_cg->builder, value, p_lhs);
}

static LLVMValueRef
cg_binary_expr(struct PCodegenLLVM* p_cg, PAstBinaryExpr* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_BINARY_EXPR);

  if (p_node->opcode == P_BINARY_LOG_AND) {
    return cg_log_and_impl(p_cg, p_node->lhs, p_node->rhs, /* is_and= */ true);
  } else if (p_node->opcode == P_BINARY_LOG_OR) {
    return cg_log_and_impl(p_cg, p_node->lhs, p_node->rhs, /* is_and= */ false);
  } else if (is_assignment(p_node->opcode)) {
    p_cg->is_in_lhs = true;
    const LLVMValueRef lhs = cg_visit(p_cg, p_node->lhs);
    p_cg->is_in_lhs = false;
    const LLVMValueRef rhs = cg_visit(p_cg, p_node->rhs);
    return cg_assignment_impl(p_cg, P_AST_GET_TYPE(p_node), p_node->opcode, lhs, rhs);
  }

  PType* type = P_AST_GET_TYPE(p_node->lhs);
  const LLVMValueRef lhs = cg_visit(p_cg, p_node->lhs);
  const LLVMValueRef rhs = cg_visit(p_cg, p_node->rhs);
  return cg_binary_expr_impl(p_cg, type, p_node->opcode, lhs, rhs);
}

static LLVMValueRef
cg_call_expr(struct PCodegenLLVM* p_cg, PAstCallExpr* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_CALL_EXPR);

  const LLVMValueRef fn = p_node->func->common._llvm_address;

  LLVMValueRef* args = malloc(sizeof(LLVMValueRef) * p_node->arg_count);
  assert(args != NULL);

  for (int i = 0; i < p_node->arg_count; ++i) {
    args[i] = cg_visit(p_cg, p_node->args[i]);
  }

  // FIXME: Use LLVMBuildCall2
  const LLVMValueRef call = LLVMBuildCall(p_cg->builder, fn, args, p_node->arg_count, "");

  free(args);

  return call;
}

static LLVMValueRef
cg_cast_expr(struct PCodegenLLVM* p_cg, PAstCastExpr* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_CAST_EXPR);

  LLVMValueRef sub_expr = cg_visit(p_cg, p_node->sub_expr);
  LLVMTypeRef target_ty = cg_to_llvm_type(P_AST_GET_TYPE(p_node));
  switch (p_node->cast_kind) {
    case P_CAST_NOOP:
      return sub_expr;
    case P_CAST_INT2INT: {
      const int from_bitwidth = p_type_get_bitwidth(P_AST_GET_TYPE(p_node->sub_expr));
      const int target_bitwidth = p_type_get_bitwidth(P_AST_GET_TYPE(p_node));

      if (from_bitwidth > target_bitwidth) {
        return LLVMBuildTrunc(p_cg->builder, sub_expr, target_ty, "");
      } else { /* from_bitwidth < target_bitwidth */
        if (p_type_is_unsigned(P_AST_GET_TYPE(p_node->sub_expr))) {
          return LLVMBuildZExt(p_cg->builder, sub_expr, target_ty, "");
        } else {
          return LLVMBuildSExt(p_cg->builder, sub_expr, target_ty, "");
        }
      }
    }
    case P_CAST_INT2FLOAT:
      if (p_type_is_unsigned(P_AST_GET_TYPE(p_node->sub_expr)))
        return LLVMBuildUIToFP(p_cg->builder, sub_expr, target_ty, "");
      else
        return LLVMBuildSIToFP(p_cg->builder, sub_expr, target_ty, "");
    case P_CAST_FLOAT2FLOAT: {
      const int from_bitwidth = p_type_get_bitwidth(P_AST_GET_TYPE(p_node->sub_expr));
      const int target_bitwidth = p_type_get_bitwidth(P_AST_GET_TYPE(p_node));

      if (from_bitwidth > target_bitwidth) {
        return LLVMBuildFPTrunc(p_cg->builder, sub_expr, target_ty, "");
      } else {
        return LLVMBuildFPExt(p_cg->builder, sub_expr, target_ty, "");
      }
    }
    case P_CAST_FLOAT2INT:
      if (p_type_is_unsigned(P_AST_GET_TYPE(p_node)))
        return LLVMBuildFPToUI(p_cg->builder, sub_expr, target_ty, "");
      else
        return LLVMBuildFPToSI(p_cg->builder, sub_expr, target_ty, "");
    case P_CAST_BOOL2INT:
      return LLVMBuildZExt(p_cg->builder, sub_expr, target_ty, "");
    case P_CAST_BOOL2FLOAT:
      return LLVMBuildUIToFP(p_cg->builder, sub_expr, target_ty, "");
    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }
}

static LLVMValueRef
cg_translation_unit(struct PCodegenLLVM* p_cg, PAstTranslationUnit* p_node)
{
  assert(P_AST_GET_KIND(p_node) == P_AST_NODE_TRANSLATION_UNIT);

  for (int i = 0; i < p_node->decl_count; ++i) {
    cg_visit_decl(p_cg, p_node->decls[i]);
  }

  return NULL;
}

static LLVMValueRef
cg_visit(struct PCodegenLLVM* p_cg, PAst* p_node)
{
  assert(p_node != NULL);

  switch (P_AST_GET_KIND(p_node)) {
#define DISPATCH(p_kind, p_type, p_func)                                                                               \
  case p_kind:                                                                                                         \
    return p_func(p_cg, (p_type*)p_node)

    DISPATCH(P_AST_NODE_TRANSLATION_UNIT, PAstTranslationUnit, cg_translation_unit);
    DISPATCH(P_AST_NODE_COMPOUND_STMT, PAstCompoundStmt, cg_compound_stmt);
    DISPATCH(P_AST_NODE_LET_STMT, PAstLetStmt, cg_let_stmt);
    DISPATCH(P_AST_NODE_RETURN_STMT, PAstReturnStmt, cg_return_stmt);
    DISPATCH(P_AST_NODE_WHILE_STMT, PAstWhileStmt, cg_while_stmt);
    DISPATCH(P_AST_NODE_IF_STMT, PAstIfStmt, cg_if_stmt);
    DISPATCH(P_AST_NODE_BOOL_LITERAL, PAstBoolLiteral, cg_bool_literal);
    DISPATCH(P_AST_NODE_INT_LITERAL, PAstIntLiteral, cg_int_literal);
    DISPATCH(P_AST_NODE_FLOAT_LITERAL, PAstFloatLiteral, cg_float_literal);
    DISPATCH(P_AST_NODE_PAREN_EXPR, PAstParenExpr, cg_paren_expr);
    DISPATCH(P_AST_NODE_DECL_REF_EXPR, PAstDeclRefExpr, cg_decl_ref_expr);
    DISPATCH(P_AST_NODE_UNARY_EXPR, PAstUnaryExpr, cg_unary_expr);
    DISPATCH(P_AST_NODE_BINARY_EXPR, PAstBinaryExpr, cg_binary_expr);
    DISPATCH(P_AST_NODE_CALL_EXPR, PAstCallExpr, cg_call_expr);
    DISPATCH(P_AST_NODE_CAST_EXPR, PAstCastExpr, cg_cast_expr);
#undef DISPATCH

    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }
}

void
p_cg_init(struct PCodegenLLVM* p_cg)
{
  assert(p_cg != NULL);

  p_cg->module = LLVMModuleCreateWithName("");
  p_cg->builder = LLVMCreateBuilder();
}

void
p_cg_destroy(struct PCodegenLLVM* p_cg)
{
  assert(p_cg != NULL);

  LLVMDisposeBuilder(p_cg->builder);
  LLVMDisposeModule(p_cg->module);
}

void
p_cg_dump(struct PCodegenLLVM* p_cg)
{
  assert(p_cg != NULL);

  LLVMDumpModule(p_cg->module);
}

void
p_cg_optimize(struct PCodegenLLVM* p_cg, int p_opt_level)
{
  assert(p_cg != NULL);
  assert(p_opt_level >= 0 && p_opt_level <= 3);

  const LLVMPassManagerBuilderRef pmb = LLVMPassManagerBuilderCreate();
  LLVMPassManagerBuilderSetOptLevel(pmb, p_opt_level);

  const LLVMPassManagerRef pass_manager = LLVMCreatePassManager();
  LLVMPassManagerBuilderPopulateModulePassManager(pmb, pass_manager);

  LLVMPassManagerBuilderDispose(pmb);

  LLVMRunPassManager(pass_manager, p_cg->module);

  LLVMDisposePassManager(pass_manager);
}

void
p_cg_compile(struct PCodegenLLVM* p_cg, PAst* p_ast)
{
  assert(p_cg != NULL);

  p_cg->is_in_lhs = false;
  cg_visit(p_cg, p_ast);

  char* msg = NULL;
  LLVMVerifyModule(p_cg->module, LLVMAbortProcessAction, &msg);
  LLVMDisposeMessage(msg);
}
