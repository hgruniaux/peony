#include "sema.h"

#include "scope.h"
#include "utils/diag.h"

#include <assert.h>
#include <stdlib.h>

void
sema_push_scope(PSema* p_s)
{
  assert(p_s != NULL);

  PScope* new_scope;
  if (p_s->current_scope_cache_idx >= P_MAX_SCOPE_CACHE) {
    new_scope = P_BUMP_ALLOC(&p_global_bump_allocator, PScope);
    p_scope_init(new_scope, p_s->current_scope);
  } else {
    new_scope = p_s->scope_cache[p_s->current_scope_cache_idx];
    if (new_scope == NULL) {
      new_scope = P_BUMP_ALLOC(&p_global_bump_allocator, PScope);
      p_s->scope_cache[p_s->current_scope_cache_idx] = new_scope;
      p_scope_init(new_scope, p_s->current_scope);
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

void
sema_create_func_type(PSema* p_s, PDeclFunction* p_node, PType* return_type)
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

  p_s->curr_func_type = P_DECL_GET_TYPE(p_node);
}

bool
sema_check_func_decl(PSema* p_s, PDeclFunction* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_DECL_GET_KIND(p_node) == P_DECL_FUNCTION);

  PIdentifierInfo* name = P_DECL_GET_NAME(p_node);
  PSymbol* symbol = sema_local_lookup(p_s, name);
  if (symbol != NULL) {
    error("redeclaration of function '%i'", p_node);
    p_s->error_count++;
    return true;
  }

  bool has_error = false;
  bool found_first_default_arg = false;
  for (int i = 0; i < p_node->param_count; ++i) {
    if (p_node->params[i]->default_expr == NULL) {
      if (found_first_default_arg) {
        error("default argument missing for parameter %d of function '%i'", i + 1, name);
        has_error = true;
        p_s->error_count++;
      }
    } else {
      found_first_default_arg = true;
    }
  }

  symbol = p_scope_add_symbol(p_s->current_scope, name);
  symbol->decl = (PDecl*)p_node;

  return has_error;
}

bool
sema_check_param_decl(PSema* p_s, PDeclParam* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_DECL_GET_KIND(p_node) == P_DECL_PARAM);

  PIdentifierInfo* name = P_DECL_GET_NAME(p_node);
  PSymbol* symbol = sema_local_lookup(p_s, name);
  if (symbol != NULL) {
    error("redeclaration of parameter '%i'", p_node);
    p_s->error_count++;
    return true;
  }

  bool has_error = false;

  if (p_node->default_expr != NULL && P_AST_GET_TYPE(p_node->default_expr) != P_DECL_GET_TYPE(p_node)) {
    error("expected '%ty', found '%ty'", P_DECL_GET_TYPE(p_node), P_AST_GET_TYPE(p_node->default_expr));
    has_error = true;
    p_s->error_count++;
  }

  symbol = p_scope_add_symbol(p_s->current_scope, name);
  symbol->decl = (PDecl*)p_node;

  return has_error;
}

bool
sema_check_break_stmt(PSema* p_s, PAstBreakStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_BREAK_STMT);

  return false;
}

bool
sema_check_continue_stmt(PSema* p_s, PAstContinueStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_CONTINUE_STMT);

  return false;
}

bool
sema_check_return_stmt(PSema* p_s, PAstReturnStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_RETURN_STMT);

  bool has_error = false;

  if (p_node->ret_expr != NULL) {
    if (p_s->curr_func_type->func_ty.ret != P_AST_GET_TYPE(p_node->ret_expr)) {
      error("expected '%ty', found '%ty'", p_s->curr_func_type->func_ty.ret, P_AST_GET_TYPE(p_node->ret_expr));
      has_error = true;
      p_s->error_count++;
    }
  } else {
    if (!p_type_is_void(p_s->curr_func_type->func_ty.ret)) {
      error("expected '%ty', found '%ty'", p_type_get_void(), P_AST_GET_TYPE(p_node->ret_expr));
      has_error = true;
      p_s->error_count++;
    }
  }

  return has_error;
}

static void
check_suspicious_condition_expr(PSema* p_s, PAst* p_cond_expr)
{
  if (P_AST_GET_KIND(p_cond_expr) != P_AST_NODE_BINARY_EXPR)
    return;

  PAstBinaryExpr* bin_cond_expr = (PAstBinaryExpr*)p_cond_expr;
  bool has_warning = false;
  if (bin_cond_expr->opcode == P_BINARY_ASSIGN) {
    warning("suspicious use of operator '=', did you mean '=='?");
    p_s->warning_count++;
    has_warning = true;
  } else if (bin_cond_expr->opcode == P_BINARY_ASSIGN_BIT_OR) {
    warning("suspicious use of operator '|=', did you mean '!='?");
    p_s->warning_count++;
    has_warning = true;
  }

  if (has_warning) {
    note("add parenthesis around condition to ignore above warning");
  }
}

bool
sema_check_if_stmt(PSema* p_s, PAstIfStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_IF_STMT);

  assert(p_node->cond_expr != NULL);
  assert(p_node->then_stmt != NULL);

  bool has_error = false;

  if (!p_type_is_bool(P_AST_GET_TYPE(p_node->cond_expr))) {
    error("expected '%ty', found '%ty'", p_type_get_bool(), P_AST_GET_TYPE(p_node->cond_expr));
    has_error = true;
    p_s->error_count++;
  }

  check_suspicious_condition_expr(p_s, p_node->cond_expr);

  return has_error;
}

bool
sema_check_while_stmt(PSema* p_s, PAstWhileStmt* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_WHILE_STMT);

  assert(p_node->cond_expr != NULL);
  assert(p_node->body_stmt != NULL);

  bool has_error = false;

  if (!p_type_is_bool(P_AST_GET_TYPE(p_node->cond_expr))) {
    error("expected '%ty', found '%ty'", p_type_get_bool(), P_AST_GET_TYPE(p_node->cond_expr));
    has_error = true;
    p_s->error_count++;
  }

  check_suspicious_condition_expr(p_s, p_node->cond_expr);

  return has_error;
}

bool
sema_check_int_literal(PSema* p_s, PAstIntLiteral* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_INT_LITERAL);

  bool has_error = false;

  uintmax_t value = p_node->value;

  uintmax_t max_value = UINTMAX_MAX;
  switch (P_AST_GET_TYPE(p_node)->kind) {
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
    default:
      assert(false && "int literal must have an integer type");
      break;
  }

  if (value > max_value) {
    error("integer literal too big for its type '%ty'", P_AST_GET_TYPE(p_node));
    has_error = true;
    p_s->error_count++;
  }

  return has_error;
}

bool
sema_check_unary_expr(PSema* p_s, PAstUnaryExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_UNARY_EXPR);

  assert(p_node->sub_expr != NULL);

  bool has_error = false;

  PType* type = P_AST_GET_TYPE(p_node->sub_expr);
  P_AST_GET_TYPE(p_node) = type;

  if ((p_node->opcode == P_UNARY_NEG && !p_type_is_float(type) && !p_type_is_signed(type)) ||
      (p_node->opcode == P_UNARY_NOT && !p_type_is_int(type) && !p_type_is_bool(type))) {
    error("could not apply unary operator '-' to type '%ty'", type);
    p_s->error_count++;
    has_error = true;
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

  switch (p_node->opcode) {
    case P_BINARY_LOG_AND:
    case P_BINARY_LOG_OR:
      if (!p_type_is_bool(P_AST_GET_TYPE(p_node->lhs))) {
        error("expected '%ty', found '%ty'", p_type_get_bool(), P_AST_GET_TYPE(p_node->lhs));
        has_error = true;
        p_s->error_count++;
      }

      if (!p_type_is_bool(P_AST_GET_TYPE(p_node->rhs))) {
        error("expected '%ty', found '%ty'", p_type_get_bool(), P_AST_GET_TYPE(p_node->lhs));
        has_error = true;
        p_s->error_count++;
      }

      HEDLEY_FALL_THROUGH;
    case P_BINARY_EQ:
    case P_BINARY_NE:
    case P_BINARY_LT:
    case P_BINARY_LE:
    case P_BINARY_GT:
    case P_BINARY_GE:
      P_AST_GET_TYPE(p_node) = p_type_get_bool();
      break;

    case P_BINARY_BIT_AND:
    case P_BINARY_BIT_XOR:
    case P_BINARY_BIT_OR:
    case P_BINARY_ASSIGN_BIT_AND:
    case P_BINARY_ASSIGN_BIT_XOR:
    case P_BINARY_ASSIGN_BIT_OR:
      if (!p_type_is_int(P_AST_GET_TYPE(p_node->lhs))) {
        error("expected integer type, found '%ty'", P_AST_GET_TYPE(p_node->lhs));
        has_error = true;
        p_s->error_count++;
      }

      if (!p_type_is_int(P_AST_GET_TYPE(p_node->rhs))) {
        error("expected integer type, found '%ty'", P_AST_GET_TYPE(p_node->rhs));
        has_error = true;
        p_s->error_count++;
      }

      HEDLEY_FALL_THROUGH;
    default:
      if (P_AST_GET_TYPE(p_node->lhs) != P_AST_GET_TYPE(p_node->rhs)) {
        error("type mismatch, got '%ty' for LHS and '%ty' for RHS",
              P_AST_GET_TYPE(p_node->lhs),
              P_AST_GET_TYPE(p_node->rhs));
        has_error = true;
        p_s->error_count++;
      }

      P_AST_GET_TYPE(p_node) = P_AST_GET_TYPE(p_node->lhs);
      break;
  }

  return has_error;
}

bool
sema_check_cast_expr(PSema* p_s, PAstCastExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_CAST_EXPR);

  assert(p_node->sub_expr != NULL);
  assert(P_AST_GET_TYPE(p_node) != NULL);

  PType* from_ty = P_AST_GET_TYPE(p_node->sub_expr);
  PType* target_ty = P_AST_GET_TYPE(p_node);

  if (from_ty == target_ty) {
    warning("no-op cast operator, expression already have type '%ty'", from_ty);
    p_s->warning_count++;
    p_node->cast_kind = P_CAST_NOOP;
    return false;
  }

  p_node->cast_kind = P_CAST_NOOP;

  if (p_type_is_int(from_ty)) {
    if (p_type_is_float(target_ty)) {
      p_node->cast_kind = P_CAST_INT2FLOAT;
    } else if (p_type_is_int(target_ty)) {
      if (p_type_get_bitwidth(from_ty) == p_type_get_bitwidth(target_ty))
        p_node->cast_kind = P_CAST_NOOP; /* just a signed <-> unsigned conversion */
      else
        p_node->cast_kind = P_CAST_INT2INT;
    }
  } else if (p_type_is_float(from_ty)) {
    if (p_type_is_int(target_ty))
      p_node->cast_kind = P_CAST_FLOAT2INT;
    else if (p_type_is_float(target_ty))
      p_node->cast_kind = P_CAST_FLOAT2FLOAT;
  } else if (p_type_is_bool(from_ty)) {
    if (p_type_is_int(target_ty))
      p_node->cast_kind = P_CAST_BOOL2INT;
    else if (p_type_is_float(target_ty))
      p_node->cast_kind = P_CAST_BOOL2FLOAT;
  }

  if (p_node->cast_kind == P_CAST_NOOP) {
    error("invalid conversion from '%ty' to '%ty'", from_ty, target_ty);
    p_s->error_count++;
    return true;
  }

  return false;
}
