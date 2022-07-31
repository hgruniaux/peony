#include "sema.h"

#include "scope.h"
#include "utils/bump_allocator.h"
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

/* Adds a PAstLValueToRValue node if the given node is not a rvalue. */
static PAst*
convert_to_rvalue(PAst* p_node)
{
  if (P_AST_EXPR_IS_RVALUE(p_node))
    return p_node; /* no conversion needed */

  PAstLValueToRValueExpr* node = CREATE_NODE(PAstLValueToRValueExpr, P_AST_NODE_LVALUE_TO_RVALUE_EXPR);
  node->sub_expr = p_node;
  P_AST_EXPR_GET_TYPE(node) = P_AST_EXPR_GET_TYPE(p_node);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_VC_RVALUE;
  return (PAst*)node;
}

/* Returns true if p_from is trivially convertible to p_to.
 * Most of time this is equivalent to p_from == p_to, unless
 * if one of the types is a generic int/float. */
static bool
are_types_compatible(PType* p_from, PType* p_to)
{
  if (p_from == p_to)
    return true;

  if ((p_type_is_generic_int(p_from) && p_type_is_int(p_to)) || (p_type_is_int(p_to) && p_type_is_generic_int(p_to)))
    return true;
  if ((p_type_is_generic_float(p_from) && p_type_is_float(p_to)) ||
      (p_type_is_float(p_to) && p_type_is_generic_float(p_to)))
    return true;

  return false;
}

static void
resolve_generic_type(PAst* p_node, PType* p_hint)
{
  assert(p_type_is_generic(P_AST_EXPR_GET_TYPE(p_node)));

  if (p_hint == NULL) {
    if (p_type_is_generic_int(P_AST_EXPR_GET_TYPE(p_node)))
      p_hint = p_type_get_i32();
    else
      p_hint = p_type_get_f32();
  }

  P_AST_EXPR_GET_TYPE(p_node) = p_hint;

  switch (P_AST_GET_KIND(p_node)) {
    case P_AST_NODE_PAREN_EXPR:
      resolve_generic_type(((PAstParenExpr*)p_node)->sub_expr, p_hint);
      break;
    case P_AST_NODE_UNARY_EXPR:
      resolve_generic_type(((PAstUnaryExpr*)p_node)->sub_expr, p_hint);
      break;
    case P_AST_NODE_CAST_EXPR:
      resolve_generic_type(((PAstCastExpr*)p_node)->sub_expr, p_hint);
      break;
    default:
      break;
  }
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
    error("redeclaration of function '%i'", p_node);
    p_s->error_count++;
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
        error("default argument missing for parameter %d of function '%i'", i + 1, name);
        has_error = true;
        p_s->error_count++;
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
    error("redeclaration of parameter '%i'", p_node);
    p_s->error_count++;
    return true;
  }

  bool has_error = false;

  if (p_node->default_expr != NULL && P_AST_EXPR_GET_TYPE(p_node->default_expr) != P_DECL_GET_TYPE(p_node)) {
    error("expected '%ty', found '%ty'", P_DECL_GET_TYPE(p_node), P_AST_EXPR_GET_TYPE(p_node->default_expr));
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
    if (!are_types_compatible(P_AST_EXPR_GET_TYPE(p_node->ret_expr), p_s->curr_func_type->ret_type)) {
      error("expected '%ty', found '%ty'", p_s->curr_func_type->ret_type, P_AST_EXPR_GET_TYPE(p_node->ret_expr));
      has_error = true;
      p_s->error_count++;
    }

    if (p_type_is_generic(P_AST_EXPR_GET_TYPE(p_node->ret_expr))) {
      resolve_generic_type(p_node->ret_expr, p_s->curr_func_type->ret_type);
    }

    p_node->ret_expr = convert_to_rvalue(p_node->ret_expr);
  } else {
    if (!p_type_is_void(p_s->curr_func_type->ret_type)) {
      error("expected '%ty', found '%ty'", p_type_get_void(), P_AST_EXPR_GET_TYPE(p_node->ret_expr));
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

  if (!p_type_is_bool(P_AST_EXPR_GET_TYPE(p_node->cond_expr))) {
    error("expected '%ty', found '%ty'", p_type_get_bool(), P_AST_EXPR_GET_TYPE(p_node->cond_expr));
    has_error = true;
    p_s->error_count++;
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

  if (!p_type_is_bool(P_AST_EXPR_GET_TYPE(p_node->cond_expr))) {
    error("expected '%ty', found '%ty'", p_type_get_bool(), P_AST_EXPR_GET_TYPE(p_node->cond_expr));
    has_error = true;
    p_s->error_count++;
  }

  check_suspicious_condition_expr(p_s, p_node->cond_expr);

  p_node->cond_expr = convert_to_rvalue(p_node->cond_expr);
  return has_error;
}

bool
sema_check_bool_literal(PSema* p_s, PAstBoolLiteral* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_BOOL_LITERAL);

  P_AST_EXPR_GET_TYPE(p_node) = p_type_get_bool();
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
  switch (P_TYPE_GET_KIND(P_AST_EXPR_GET_TYPE(p_node))) {
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
    error("integer literal too large for its type '%ty'", P_AST_EXPR_GET_TYPE(p_node));
    has_error = true;
    p_s->error_count++;
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
    error("use of undeclared variable '%i'", p_name);
    has_error = true;
    p_s->error_count++;
  } else {
    decl = symbol->decl;
    P_AST_EXPR_GET_TYPE(p_node) = P_DECL_GET_TYPE(decl);
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

  P_AST_EXPR_GET_TYPE(p_node) = P_AST_EXPR_GET_TYPE(p_node->sub_expr);
  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_AST_EXPR_GET_VALUE_CATEGORY(p_node->sub_expr);
  return false;
}

bool
sema_check_unary_expr(PSema* p_s, PAstUnaryExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_UNARY_EXPR);

  assert(p_node->sub_expr != NULL);

  bool has_error = false;

  PType* type = P_AST_EXPR_GET_TYPE(p_node->sub_expr);
  P_AST_EXPR_GET_TYPE(p_node) = type;

  switch (p_node->opcode) {
    case P_UNARY_NEG:
      if (!p_type_is_float(type) && !p_type_is_signed(type)) {
        error("cannot not apply unary operator '-' to type '%ty'", type);
        p_s->error_count++;
        has_error = true;
      }

      p_node->sub_expr = convert_to_rvalue(p_node->sub_expr);
      P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
      break;
    case P_UNARY_NOT:
      if (!p_type_is_bool(type) && !p_type_is_int(type)) {
        error("cannot not apply unary operator '!' to type '%ty'", type);
        p_s->error_count++;
        has_error = true;
      }

      p_node->sub_expr = convert_to_rvalue(p_node->sub_expr);
      P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
      break;
    case P_UNARY_ADDRESS_OF:
      if (P_AST_EXPR_IS_RVALUE(p_node->sub_expr)) {
        error("could not take address of an rvalue of type '%ty'", type);
        p_s->error_count++;
        has_error = true;
      }

      P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
      P_AST_EXPR_GET_TYPE(p_node) = p_type_get_pointer(type);
      break;
    case P_UNARY_DEREF:
      if (!p_type_is_pointer(type)) {
        error("indirection requires pointer operand ('%ty' invalid)", type);
        p_s->error_count++;
        has_error = true;
      } else {
        P_AST_EXPR_GET_TYPE(p_node) = ((PPointerType*)type)->element_type;
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

  switch (p_node->opcode) {
    case P_BINARY_LOG_AND:
    case P_BINARY_LOG_OR:
      if (!p_type_is_bool(P_AST_EXPR_GET_TYPE(p_node->lhs))) {
        error("expected '%ty', found '%ty'", p_type_get_bool(), P_AST_EXPR_GET_TYPE(p_node->lhs));
        has_error = true;
        p_s->error_count++;
      }

      if (!p_type_is_bool(P_AST_EXPR_GET_TYPE(p_node->rhs))) {
        error("expected '%ty', found '%ty'", p_type_get_bool(), P_AST_EXPR_GET_TYPE(p_node->lhs));
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
      P_AST_EXPR_GET_TYPE(p_node) = p_type_get_bool();
      break;

    case P_BINARY_BIT_AND:
    case P_BINARY_BIT_XOR:
    case P_BINARY_BIT_OR:
    case P_BINARY_ASSIGN_BIT_AND:
    case P_BINARY_ASSIGN_BIT_XOR:
    case P_BINARY_ASSIGN_BIT_OR:
      if (!p_type_is_int(P_AST_EXPR_GET_TYPE(p_node->lhs))) {
        error("expected integer type, found '%ty'", P_AST_EXPR_GET_TYPE(p_node->lhs));
        has_error = true;
        p_s->error_count++;
      }

      if (!p_type_is_int(P_AST_EXPR_GET_TYPE(p_node->rhs))) {
        error("expected integer type, found '%ty'", P_AST_EXPR_GET_TYPE(p_node->rhs));
        has_error = true;
        p_s->error_count++;
      }

      HEDLEY_FALL_THROUGH;
    default:
      if (!are_types_compatible(P_AST_EXPR_GET_TYPE(p_node->lhs), P_AST_EXPR_GET_TYPE(p_node->rhs))) {
        error("type mismatch, got '%ty' for LHS and '%ty' for RHS",
              P_AST_EXPR_GET_TYPE(p_node->lhs),
              P_AST_EXPR_GET_TYPE(p_node->rhs));
        has_error = true;
        p_s->error_count++;
      }

      if (!p_type_is_arithmetic(P_AST_EXPR_GET_TYPE(p_node->lhs))) {
        error("expected arithmetic type (either integer or float), found '%ty'", P_AST_EXPR_GET_TYPE(p_node->lhs));
        has_error = true;
        p_s->error_count++;
      }

      /* Resolve generic types. */
      if (p_type_is_generic(P_AST_EXPR_GET_TYPE(p_node->lhs)) && !p_type_is_generic(P_AST_EXPR_GET_TYPE(p_node->rhs))) {
        resolve_generic_type(p_node->lhs, P_AST_EXPR_GET_TYPE(p_node->rhs));
      } else if (!p_type_is_generic(P_AST_EXPR_GET_TYPE(p_node->lhs)) &&
                 p_type_is_generic(P_AST_EXPR_GET_TYPE(p_node->rhs))) {
        resolve_generic_type(p_node->rhs, P_AST_EXPR_GET_TYPE(p_node->lhs));
      }

      P_AST_EXPR_GET_TYPE(p_node) = P_AST_EXPR_GET_TYPE(p_node->lhs);
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

  PFunctionType* func_type = ((PFunctionType*)P_AST_EXPR_GET_TYPE(p_node->callee));

  if (func_type->arg_count != p_node->arg_count) {
    error("expected %d arguments, but got %d arguments", func_type->arg_count, p_node->arg_count);
    p_s->error_count++;
    has_error = true;
  } else {
    /* Check arguments type */
    for (int i = 0; i < p_node->arg_count; ++i) {
      if (!are_types_compatible(P_AST_EXPR_GET_TYPE(p_node->args[i]), func_type->args[i])) {
        error("expected '%ty', found '%ty'", func_type->args[i], P_AST_EXPR_GET_TYPE(p_node->args[i]));
        p_s->error_count++;
        has_error = true;
      } else if (p_type_is_generic(p_node->args[i])) {
        resolve_generic_type(p_node->args[i], func_type->args[i]);
      }
    }
  }

  P_AST_EXPR_GET_TYPE(p_node) = func_type->ret_type;
  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_VC_RVALUE;
  return has_error;
}

bool
sema_check_cast_expr(PSema* p_s, PAstCastExpr* p_node)
{
  assert(p_s != NULL && p_node != NULL && P_AST_GET_KIND(p_node) == P_AST_NODE_CAST_EXPR);

  assert(p_node->sub_expr != NULL);
  assert(P_AST_EXPR_GET_TYPE(p_node) != NULL);

  PType* from_ty = P_AST_EXPR_GET_TYPE(p_node->sub_expr);
  PType* target_ty = P_AST_EXPR_GET_TYPE(p_node);

  P_AST_EXPR_GET_VALUE_CATEGORY(p_node) = P_AST_EXPR_GET_VALUE_CATEGORY(p_node->sub_expr);

  if (from_ty == target_ty) {
    warning("no-op cast operator, expression already have type '%ty'", from_ty);
    p_s->warning_count++;
    p_node->cast_kind = P_CAST_NOOP;
    return false;
  }

  bool valid_cast = true;
  if (p_type_is_int(from_ty)) {
    if (p_type_is_generic_int(p_node)) {
      /* Resolve generic integer with the biggest int type. */
      resolve_generic_type((PAst*)p_node, p_type_get_i64());
    }

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
    if (p_type_is_generic_float(p_node)) {
      /* Resolve generic float with the biggest float type. */
      resolve_generic_type((PAst*)p_node, p_type_get_f64());
    }

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
    error("invalid conversion from '%ty' to '%ty'", from_ty, target_ty);
    p_s->error_count++;
    return true;
  }

  return false;
}
