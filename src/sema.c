#include "sema.h"

#include "utils/bump_allocator.h"
#include "utils/diag.h"

#include <assert.h>
#include <stdlib.h>

void
sema_push_scope(PSema* p_s, PScopeFlags p_flags)
{
  assert(p_s != NULL);

  PScope* new_scope;
  if (p_s->current_scope_cache_idx >= P_MAX_SCOPE_CACHE) {
    new_scope = P_BUMP_ALLOC(&p_global_bump_allocator, PScope);
    p_scope_init(new_scope, p_s->current_scope, p_flags);
  } else {
    new_scope = p_s->scope_cache[p_s->current_scope_cache_idx];
    if (new_scope == NULL) {
      new_scope = P_BUMP_ALLOC(&p_global_bump_allocator, PScope);
      p_s->scope_cache[p_s->current_scope_cache_idx] = new_scope;
      p_scope_init(new_scope, p_s->current_scope, p_flags);
    }
  }

  p_s->current_scope = new_scope;
  p_s->current_scope_cache_idx++;
}

void
sema_pop_scope(PSema* p_s)
{
  assert(p_s != NULL);
  assert(p_s->current_scope_cache_idx > 0); /* check stack underflow */

  PScope* parent_scope = p_s->current_scope->parent_scope;

  if (p_s->current_scope_cache_idx > P_MAX_SCOPE_CACHE) {
    p_scope_destroy(p_s->current_scope);
  } else {
    p_scope_clear(p_s->current_scope);
  }

  p_s->current_scope = parent_scope;
  p_s->current_scope_cache_idx--;
}

PSymbol*
sema_local_lookup(PSema* p_s, PIdentifierInfo* p_name)
{
  assert(p_s != NULL && p_name != NULL);

  return p_scope_local_lookup(p_s->current_scope, p_name);
}

PSymbol*
sema_lookup(PSema* p_s, PIdentifierInfo* p_name)
{
  assert(p_s != NULL && p_name != NULL);

  PScope* scope = p_s->current_scope;
  do {
    PSymbol* symbol = p_scope_local_lookup(scope, p_name);
    if (symbol != NULL)
      return symbol;
    scope = scope->parent_scope;
  } while (scope != NULL);

  return NULL;
}

/* Adds a PAstLValueToRValue node if the given node is not a rvalue. */
static PAst*
convert_to_rvalue(PAst* p_node)
{
  if (P_AST_EXPR_IS_RVALUE(p_node))
    return p_node; /* no conversion needed */

  PAstLValueToRValueExpr* node = CREATE_NODE(PAstLValueToRValueExpr, P_AST_NODE_LVALUE_TO_RVALUE_EXPR);
  node->sub_expr = p_node;
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_VC_RVALUE;
  return (PAst*)node;
}

/* Returns true if p_from is trivially convertible to p_to.
 * Most of time this is equivalent to p_from == p_to, unless
 * if one of the types is a generic int/float. */
static bool
are_types_compatible(PType* p_from, PType* p_to)
{
  p_from = p_type_get_canonical(p_from);
  p_to = p_type_get_canonical(p_to);

  if (p_from == p_to)
    return true;

  if ((p_type_is_generic_int(p_from) && p_type_is_int(p_to)) || (p_type_is_int(p_to) && p_type_is_generic_int(p_to)))
    return true;
  if ((p_type_is_generic_float(p_from) && p_type_is_float(p_to)) ||
      (p_type_is_float(p_to) && p_type_is_generic_float(p_to)))
    return true;

  return false;
}

bool
sema_check_pre_func_decl(PSema* p_s, PDeclFunction* p_node, PType* return_type)
{
  assert(p_s != NULL && p_node != NULL && P_DECL_GET_KIND(p_node) == P_DECL_FUNCTION);

  if (return_type == NULL)
    return_type = p_type_get_void();

  PType** param_types = malloc(sizeof(PType*) * p_node->param_count);
  assert(param_types != NULL);
  for (int i = 0; i < p_node->param_count; ++i)
    param_types[i] = P_DECL_GET_TYPE(p_node->params[i]);
  P_DECL_GET_TYPE(p_node) = p_type_get_function(return_type, param_types, p_node->param_count);
  free(param_types);

  p_s->curr_func_type = (PFunctionType*)P_DECL_GET_TYPE(p_node);

  PIdentifierInfo* name = P_DECL_GET_NAME(p_node);
  PSymbol* symbol = sema_local_lookup(p_s, name);
  if (symbol != NULL) {
    // FIXME: provide better source location
    PDiag* d = diag(P_DK_err_redeclaration_function);
    diag_add_arg_ident(d, name);
    diag_flush(d);
    return true;
  }

  symbol = p_scope_add_symbol(p_s->current_scope, name);
  symbol->decl = (PDecl*)p_node;
  return false;
}

bool
sema_check_func_decl(PSema* p_s, PDeclFunction* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_DECL_GET_KIND(p_node) == P_DECL_FUNCTION);

  PIdentifierInfo* name = P_DECL_GET_NAME(p_node);

  bool has_error = false;
  bool found_first_default_arg = false;
  for (int i = 0; i < p_node->param_count; ++i) {
    if (p_node->params[i]->default_expr == NULL) {
      if (found_first_default_arg) {
        PDiag* d = diag_at(P_DK_err_missing_default_argument, P_AST_GET_SOURCE_RANGE(p_node->params).end);
        diag_add_arg_int(d, i + 1);
        diag_add_arg_ident(d, name);
        diag_flush(d);
        has_error = true;
      }
    } else {
      found_first_default_arg = true;
    }
  }

  return has_error;
}

bool
sema_check_param_decl(PSema* p_s, PDeclParam* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_DECL_GET_KIND(p_node) == P_DECL_PARAM);

  PIdentifierInfo* name = P_DECL_GET_NAME(p_node);
  PSymbol* symbol = sema_local_lookup(p_s, name);
  if (symbol != NULL) {
    // FIXME: provide better source location
    PDiag* d = diag(P_DK_err_parameter_name_already_used);
    diag_add_arg_ident(d, name);
    diag_flush(d);
    
    return true;
  }

  bool has_error = false;

  if (p_node->default_expr != NULL && p_ast_get_type(p_node->default_expr) != P_DECL_GET_TYPE(p_node)) {
    PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_node->default_expr).begin);
    diag_add_arg_type(d, P_DECL_GET_TYPE(p_node));
    diag_add_arg_type(d, p_ast_get_type(p_node->default_expr));
    diag_flush(d);
    has_error = true;
  }

  symbol = p_scope_add_symbol(p_s->current_scope, name);
  symbol->decl = (PDecl*)p_node;

  return has_error;
}

static PScope*
find_nearest_scope_with_flag(PSema* p_s, PScopeFlags p_flag)
{
  PScope* scope = p_s->current_scope;
  while (scope != NULL) {
    if (scope->flags & p_flag)
      return scope;

    scope = scope->parent_scope;
  }

  return NULL;
}

bool
sema_check_break_stmt(PSema* p_s, PAstBreakStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_BREAK_STMT);

  p_node->loop_target = NULL;

  PScope* break_scope = find_nearest_scope_with_flag(p_s, P_SF_BREAK);
  if (break_scope == NULL) {
    PDiag* d = diag_at(P_DK_err_break_or_continue_outside_of_loop, P_AST_GET_SOURCE_RANGE(p_node).begin);
    diag_add_arg_str(d, "break");
    diag_flush(d);
    return true;
  } else {
    p_node->loop_target = break_scope->statement;
  }

  return false;
}

bool
sema_check_continue_stmt(PSema* p_s, PAstContinueStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_CONTINUE_STMT);

  p_node->loop_target = NULL;

  PScope* continue_scope = find_nearest_scope_with_flag(p_s, P_SF_CONTINUE);
  if (continue_scope == NULL) {
    PDiag* d = diag_at(P_DK_err_break_or_continue_outside_of_loop, P_AST_GET_SOURCE_RANGE(p_node).begin);
    diag_add_arg_str(d, "continue");
    diag_flush(d);
    return true;
  } else {
    p_node->loop_target = continue_scope->statement;
  }

  return false;
}

bool
sema_check_return_stmt(PSema* p_s, PAstReturnStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_RETURN_STMT);

  bool has_error = false;

  if (p_node->ret_expr != NULL) {
    PType* ret_type = p_ast_get_type(p_node->ret_expr);
    if (!are_types_compatible(ret_type, p_s->curr_func_type->ret_type)) {
      PDiag* d = diag(P_DK_err_expected_type);
      diag_add_arg_type(d, p_s->curr_func_type->ret_type);
      diag_add_arg_type(d, ret_type);
      diag_flush(d);
      has_error = true;
    }

    p_node->ret_expr = convert_to_rvalue(p_node->ret_expr);
  } else {
    if (!p_type_is_void(p_s->curr_func_type->ret_type)) {
      PDiag* d = diag(P_DK_err_expected_type);
      diag_add_arg_type(d, p_type_get_void());
      diag_add_arg_type(d, p_ast_get_type(p_node->ret_expr));
      diag_flush(d);
      has_error = true;
    }
  }

  return has_error;
}

static void
check_suspicious_condition_expr(PSema* p_s, PAst* p_cond_expr)
{
  if (P_AST_GET_KIND(p_cond_expr) != P_AST_NODE_BINARY_EXPR)
    return;

    // TODO: reimplement this warning
#if 0
  PAstBinaryExpr* bin_cond_expr = (PAstBinaryExpr*)p_cond_expr;
  bool has_warning = false;
  if (bin_cond_expr->opcode == P_BINARY_ASSIGN) {
    warning("suspicious use of operator '=', did you mean '=='?");
    has_warning = true;
  } else if (bin_cond_expr->opcode == P_BINARY_ASSIGN_BIT_OR) {
    warning("suspicious use of operator '|=', did you mean '!='?");
    has_warning = true;
  }

  if (has_warning) {
    note("add parenthesis around condition to ignore above warning");
  }
#endif
}

bool
sema_check_if_stmt(PSema* p_s, PAstIfStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_IF_STMT);

  assert(p_node->cond_expr != NULL);
  assert(p_node->then_stmt != NULL);

  bool has_error = false;

  PType* cond_type = p_ast_get_type(p_node->cond_expr);
  if (!p_type_is_bool(cond_type)) {
    PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_node->cond_expr).begin);
    diag_add_arg_type(d, p_type_get_bool());
    diag_add_arg_type(d, cond_type);
    diag_flush(d);
    has_error = true;
  }

  check_suspicious_condition_expr(p_s, p_node->cond_expr);

  p_node->cond_expr = convert_to_rvalue(p_node->cond_expr);
  return has_error;
}

bool
sema_check_while_stmt(PSema* p_s, PAstWhileStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_WHILE_STMT);

  assert(p_node->cond_expr != NULL);
  assert(p_node->body_stmt != NULL);

  bool has_error = false;

  PType* cond_type = p_ast_get_type(p_node->cond_expr);
  if (!p_type_is_bool(cond_type)) {
    PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_node->cond_expr).begin);
    diag_add_arg_type(d, p_type_get_bool());
    diag_add_arg_type(d, cond_type);
    diag_flush(d);
    has_error = true;
  }

  check_suspicious_condition_expr(p_s, p_node->cond_expr);

  p_node->cond_expr = convert_to_rvalue(p_node->cond_expr);
  return has_error;
}

bool
sema_check_bool_literal(PSema* p_s, PAstBoolLiteral* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_BOOL_LITERAL);

  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
  return false;
}

bool
sema_check_int_literal(PSema* p_s, PAstIntLiteral* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_INT_LITERAL);

  bool has_error = false;

  uintmax_t value = p_node->value;

  uintmax_t max_value = UINTMAX_MAX;
  switch (P_TYPE_GET_KIND(p_node->type)) {
    case P_TYPE_I8:
      max_value = INT8_MAX;
      break;
    case P_TYPE_I16:
      max_value = INT16_MAX;
      break;
    case P_TYPE_I32:
      max_value = INT32_MAX;
      break;
    case P_TYPE_I64:
      max_value = INT64_MAX;
      break;
    case P_TYPE_U8:
      max_value = UINT8_MAX;
      break;
    case P_TYPE_U16:
      max_value = UINT16_MAX;
      break;
    case P_TYPE_U32:
      max_value = UINT32_MAX;
      break;
    case P_TYPE_U64:
      max_value = UINT64_MAX;
      break;
    case P_TYPE_GENERIC_INT:
      max_value = UINTMAX_MAX;
      break;
    default:
      assert(false && "int literal must have an integer type");
      break;
  }

  if (value > max_value) {
    PDiag* d = diag_at(P_DK_err_int_literal_too_large, P_AST_GET_SOURCE_RANGE(p_node).begin);
    diag_flush(d);
    has_error = true;
  }

  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
  return has_error;
}

bool
sema_check_float_literal(PSema* p_s, PAstFloatLiteral* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_FLOAT_LITERAL);

  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
  return false;
}

bool
sema_check_decl_ref_expr(PSema* p_s, PAstDeclRefExpr* p_node, PIdentifierInfo* p_name)
{
  assert(p_s != NULL && p_node != NULL && p_name != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_DECL_REF_EXPR);

  assert(p_node->decl != NULL);

  bool has_error = false;

  PDecl* decl = NULL;
  PSymbol* symbol = sema_lookup(p_s, p_name);
  if (symbol == NULL) {
    PDiag* d = diag_at(P_DK_err_use_undeclared_variable, P_AST_GET_SOURCE_RANGE(p_node).begin);
    diag_add_arg_ident(d, p_name);
    diag_flush(d);
    has_error = true;
  } else {
    decl = symbol->decl;
    decl->common.used = true;
  }

  p_node->decl = decl;

  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_LVALUE;
  return has_error;
}

bool
sema_check_paren_expr(PSema* p_s, PAstParenExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_PAREN_EXPR);

  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_AST_EXPR_GET_VALUE_CATEGORY(p_node->sub_expr);
  return false;
}

bool
sema_check_unary_expr(PSema* p_s, PAstUnaryExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_UNARY_EXPR);

  assert(p_node->sub_expr != NULL);

  bool has_error = false;

  PType* type = p_ast_get_type(p_node->sub_expr);
  p_node->type = type;

  switch (p_node->opcode) {
    case P_UNARY_NEG:
      if (!p_type_is_float(type) && !p_type_is_signed(type)) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_unary_op, p_node->op_loc);
        diag_add_arg_char(d, '-');
        diag_add_arg_type(d, type);
        diag_flush(d);
        
        has_error = true;
      }

      p_node->sub_expr = convert_to_rvalue(p_node->sub_expr);
      P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
      break;
    case P_UNARY_NOT:
      if (!p_type_is_bool(type) && !p_type_is_int(type)) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_unary_op, p_node->op_loc);
        diag_add_arg_char(d, '!');
        diag_add_arg_type(d, type);
        diag_flush(d);
        
        has_error = true;
      }

      p_node->sub_expr = convert_to_rvalue(p_node->sub_expr);
      P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
      break;
    case P_UNARY_ADDRESS_OF:
      if (P_AST_EXPR_IS_RVALUE(p_node->sub_expr)) {
        PDiag* d = diag_at(P_DK_err_could_not_take_addr_rvalue, p_node->op_loc);
        diag_add_arg_type(d, type);
        diag_flush(d);
        
        has_error = true;
      }

      P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
      p_node->type = p_type_get_pointer(type);
      break;
    case P_UNARY_DEREF:
      if (!p_type_is_pointer(type)) {
        PDiag* d = diag_at(P_DK_err_indirection_requires_ptr, p_node->op_loc);
        diag_add_arg_type(d, type);
        diag_flush(d);
        
        has_error = true;
      } else {
        p_node->type = ((PPointerType*)type)->element_type;
      }

      P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_LVALUE;
      break;
    default:
      HEDLEY_UNREACHABLE_RETURN(true);
  }

  return has_error;
}

bool
sema_check_binary_expr(PSema* p_s, PAstBinaryExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_BINARY_EXPR);

  assert(p_node->lhs != NULL);
  assert(p_node->rhs != NULL);

  bool has_error = false;

  PType* lhs_type = p_ast_get_type(p_node->lhs);
  PType* rhs_type = p_ast_get_type(p_node->rhs);

  switch (p_node->opcode) {
    case P_BINARY_LOG_AND:
    case P_BINARY_LOG_OR:
      if (!p_type_is_bool(lhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_node->lhs).begin);
        diag_add_arg_type(d, p_type_get_bool());
        diag_add_arg_type(d, lhs_type);
        diag_flush(d);
        has_error = true;
      }

      if (!p_type_is_bool(rhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_node->rhs).begin);
        diag_add_arg_type(d, p_type_get_bool());
        diag_add_arg_type(d, rhs_type);
        diag_flush(d);
        has_error = true;
      }

      HEDLEY_FALL_THROUGH;
    case P_BINARY_EQ:
    case P_BINARY_NE:
    case P_BINARY_LT:
    case P_BINARY_LE:
    case P_BINARY_GT:
    case P_BINARY_GE:
      if (!are_types_compatible(lhs_type, rhs_type)) {
        PDiag* d = diag_at(P_DK_err_cannot_bin_op_generic, p_node->op_loc);
        diag_add_arg_str(d, p_binop_get_spelling(p_node->opcode));
        diag_add_arg_type(d, lhs_type);
        diag_add_arg_type(d, rhs_type);
        diag_flush(d);
        has_error = true;
      }

      p_node->type = p_type_get_bool();
      break;

    case P_BINARY_BIT_AND:
    case P_BINARY_BIT_XOR:
    case P_BINARY_BIT_OR:
    case P_BINARY_ASSIGN_BIT_AND:
    case P_BINARY_ASSIGN_BIT_XOR:
    case P_BINARY_ASSIGN_BIT_OR:
      if (!p_type_is_int(lhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_node->lhs).begin);
        diag_add_arg_type(d, p_type_get_generic_int());
        diag_add_arg_type(d, lhs_type);
        diag_flush(d);
        has_error = true;
      }

      if (!p_type_is_int(rhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_node->rhs).begin);
        diag_add_arg_type(d, p_type_get_generic_int());
        diag_add_arg_type(d, rhs_type);
        diag_flush(d);
        has_error = true;
      }

      HEDLEY_FALL_THROUGH;
    default:
      if (!are_types_compatible(lhs_type, rhs_type) || !p_type_is_arithmetic(lhs_type)) {
        PDiagKind diag_kind = P_DK_err_cannot_bin_op_generic;
        switch (p_node->opcode) {
          case P_BINARY_ADD:
            diag_kind = P_DK_err_cannot_add;
            break;
          case P_BINARY_SUB:
            diag_kind = P_DK_err_cannot_sub;
            break;
          case P_BINARY_MUL:
            diag_kind = P_DK_err_cannot_mul;
            break;
          case P_BINARY_DIV:
            diag_kind = P_DK_err_cannot_div;
            break;
          default:
            break;
        }

        PDiag* d = diag_at(diag_kind, p_node->op_loc);
        if (diag_kind == P_DK_err_cannot_bin_op_generic) {
          const char* spelling = p_binop_get_spelling(p_node->opcode);
          diag_add_arg_str(d, spelling);
        }

        diag_add_arg_type(d, lhs_type);
        diag_add_arg_type(d, rhs_type);
        diag_flush(d);

        has_error = true;
      }

      p_node->type = lhs_type;
      break;
  }

  p_node->rhs = convert_to_rvalue(p_node->rhs);
  if (!p_binop_is_assignment(p_node->opcode)) {
    p_node->lhs = convert_to_rvalue(p_node->lhs);
    P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
  } else {
    P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_LVALUE;
  }

  return has_error;
}

bool
sema_check_call_expr(PSema* p_s, PAstCallExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_CALL_EXPR);

  bool has_error = false;

  PFunctionType* func_type = ((PFunctionType*)p_ast_get_type(p_node->callee));

  if (func_type->arg_count != p_node->arg_count) {
    PDiag* d = diag_at(P_DK_err_expected_argument_count, P_AST_GET_SOURCE_RANGE(p_node).begin);
    diag_add_arg_int(d, func_type->arg_count);
    diag_add_arg_int(d, p_node->arg_count);
    diag_flush(d);
    
    has_error = true;
  } else {
    /* Check arguments type */
    for (int i = 0; i < p_node->arg_count; ++i) {
      PType* arg_type = p_ast_get_type(p_node->args[i]);
      if (!are_types_compatible(arg_type, func_type->args[i])) {
        PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_node->args[i]).begin);
        diag_add_arg_type(d, func_type->args[i]);
        diag_add_arg_type(d, arg_type);
        diag_flush(d);
        
        has_error = true;
      }
    }
  }

  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
  return has_error;
}

bool
sema_check_cast_expr(PSema* p_s, PAstCastExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_CAST_EXPR);

  assert(p_node->sub_expr != NULL && p_node->type != NULL);

  PType* from_ty = p_ast_get_type(p_node->sub_expr);
  PType* target_ty = p_node->type;

  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_AST_EXPR_GET_VALUE_CATEGORY(p_node->sub_expr);

  // TODO: reimplement this warning
#if 0
  if (from_ty == target_ty) {
    warning("no-op cast operator, expression already have type '%ty'", from_ty);
    p_node->cast_kind = P_CAST_NOOP;
    return false;
  }
#endif

  bool valid_cast = true;
  if (p_type_is_int(from_ty)) {
    if (p_type_is_float(target_ty)) {
      p_node->cast_kind = P_CAST_INT2FLOAT;
    } else if (p_type_is_int(target_ty)) {
      if (p_type_get_bitwidth(from_ty) == p_type_get_bitwidth(target_ty))
        p_node->cast_kind = P_CAST_NOOP; /* just a signed <-> unsigned conversion */
      else
        p_node->cast_kind = P_CAST_INT2INT;
    } else {
      valid_cast = false;
    }
  } else if (p_type_is_float(from_ty)) {
    if (p_type_is_int(target_ty))
      p_node->cast_kind = P_CAST_FLOAT2INT;
    else if (p_type_is_float(target_ty))
      p_node->cast_kind = P_CAST_FLOAT2FLOAT;
    else
      valid_cast = false;
  } else if (p_type_is_bool(from_ty)) {
    if (p_type_is_int(target_ty))
      p_node->cast_kind = P_CAST_BOOL2INT;
    else if (p_type_is_float(target_ty))
      p_node->cast_kind = P_CAST_BOOL2FLOAT;
    else
      valid_cast = false;
  }

  if (!valid_cast) {
    PDiag* d = diag(P_DK_err_unsupported_conversion);
    diag_add_arg_type(d, from_ty);
    diag_add_arg_type(d, target_ty);
    diag_flush(d);
    
    return true;
  }

  return false;
}
