#include "sema.h"

#include "utils/bump_allocator.h"
#include "utils/diag.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
    }

    p_scope_init(new_scope, p_s->current_scope, p_flags);
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
  node->common.source_range = p_node->common.source_range;
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

static bool
check_func_decl_params(PIdentifierInfo* p_func_name, PDeclParam** p_params, size_t p_param_count)
{
  bool has_error = false;
  bool found_first_default_arg = false;
  for (int i = 0; i < p_param_count; ++i) {
    if (p_params[i]->default_expr == NULL) {
      if (found_first_default_arg) {
        // FIXME: provide better source location
        PDiag* d = diag(P_DK_err_missing_default_argument);
        diag_add_arg_int(d, i + 1);
        diag_add_arg_ident(d, p_func_name);
        diag_flush(d);
        has_error = true;
      }
    } else {
      found_first_default_arg = true;
    }
  }

  return !has_error;
}

static PType*
create_function_type(PSema* p_s, PType* p_ret_type, PDeclParam** p_params, size_t p_param_count)
{
  PType** param_types = malloc(sizeof(PType*) * p_param_count);
  assert(param_types != NULL);
  for (int i = 0; i < p_param_count; ++i)
    param_types[i] = P_DECL_GET_TYPE(p_params[i]);
  PType* type = p_type_get_function(p_ret_type, param_types, p_param_count);
  free(param_types);

  return type;
}

PDeclFunction*
sema_act_on_func_decl(PSema* p_s,
                      PSourceRange p_name_range,
                      PIdentifierInfo* p_name,
                      PType* p_ret_type,
                      PDeclParam** p_params,
                      size_t p_param_count)
{
  if (p_name == NULL)
    return NULL;

  PSymbol* symbol = sema_local_lookup(p_s, p_name);
  if (symbol != NULL) {
    PDiag* d = diag_at(P_DK_err_redeclaration_function, p_name_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return NULL;
  }

  if (p_ret_type == NULL)
    p_ret_type = p_type_get_void();

  if (!check_func_decl_params(p_name, p_params, p_param_count))
    return NULL;

  PType* func_type = create_function_type(p_s, p_ret_type, p_params, p_param_count);
  p_s->curr_func_type = (PFunctionType*)func_type;

  PDeclFunction* decl =
    CREATE_DECL_EXTRA_SIZE(PDeclFunction, sizeof(PDeclParam*) * (p_param_count - 1), P_DECL_FUNCTION);
  P_DECL_GET_NAME(decl) = p_name;
  P_DECL_GET_TYPE(decl) = func_type;
  decl->param_count = p_param_count;
  memcpy(decl->params, p_params, sizeof(PDeclParam*) * p_param_count);

  symbol = p_scope_add_symbol(p_s->current_scope, p_name);
  symbol->decl = (PDecl*)decl;

  return decl;
}

void
sema_begin_func_decl_body_parsing(PSema* p_s, PDeclFunction* p_decl)
{
  sema_push_scope(p_s, P_SF_NONE);

  // Register parameters on the current scope.
  // All parameters have already been checked.
  for (size_t i = 0; i < p_decl->param_count; ++i) {
    PSymbol* symbol = p_scope_add_symbol(p_s->current_scope, P_DECL_GET_NAME(p_decl->params[i]));
    symbol->decl = (PDecl*)p_decl->params[i];
  }
}

void
sema_end_func_decl_body_parsing(PSema* p_s, PDeclFunction* p_decl)
{
  sema_pop_scope(p_s);
}

PDeclStruct*
sema_act_on_struct_decl(PSema* p_s,
                        PSourceRange p_name_range,
                        PIdentifierInfo* p_name,
                        PDeclStructField** p_fields,
                        size_t p_field_count)
{
  if (p_name == NULL)
    return NULL;

  PSymbol* symbol = sema_local_lookup(p_s, p_name);
  if (symbol != NULL) {
    PDiag* d = diag_at(P_DK_err_redeclaration_struct, p_name_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return NULL;
  }

  PDeclStruct* decl =
    CREATE_DECL_EXTRA_SIZE(PDeclStruct, sizeof(PDeclStructField*) * (p_field_count - 1), P_DECL_STRUCT);
  P_DECL_GET_NAME(decl) = p_name;
  P_DECL_GET_TYPE(decl) = p_type_get_tag((PDecl*)decl);
  decl->field_count = p_field_count;
  memcpy(decl->fields, p_fields, sizeof(PDeclStructField*) * p_field_count);

  for (int i = 0; i < decl->field_count; ++i) {
    decl->fields[i]->parent = decl;
    decl->fields[i]->idx_in_parent_fields = i;
  }

  symbol = p_scope_add_symbol(p_s->current_scope, p_name);
  symbol->decl = (PDecl*)decl;

  return decl;
}

PDeclStructField*
sema_act_on_struct_field_decl(PSema* p_s, PSourceRange p_name_range, PIdentifierInfo* p_name, PType* p_type)
{
  if (p_name == NULL)
    return NULL;

  // FIXME: check for field name duplicates

  PDeclStructField* node = CREATE_DECL(PDeclStructField, P_DECL_STRUCT_FIELD);
  P_DECL_GET_NAME(node) = p_name;
  P_DECL_GET_TYPE(node) = p_type;

  return node;
}

PDeclParam*
sema_act_on_param_decl(PSema* p_s,
                       PSourceRange p_name_range,
                       PIdentifierInfo* p_name,
                       PType* p_type,
                       PAst* p_default_expr)
{
  if (p_name == NULL)
    return NULL;

  PSymbol* symbol = sema_local_lookup(p_s, p_name);
  if (symbol != NULL) {
    PDiag* d = diag_at(P_DK_err_redeclaration_function, p_name_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return NULL;
  }

  if (p_type == NULL) {
    if (p_default_expr == NULL) {
      PDiag* d = diag_at(P_DK_err_cannot_deduce_param_type, p_name_range.begin);
      diag_add_arg_ident(d, p_name);
      diag_add_source_range(d, p_name_range);
      diag_flush(d);
      return NULL;
    }

    p_type = p_ast_get_type(p_default_expr);
  } else if (p_default_expr != NULL) {
    PType* default_expr_type = p_ast_get_type(p_default_expr);
    if (!are_types_compatible(default_expr_type, p_type)) {
      PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_default_expr).begin);
      diag_add_arg_type(d, p_type);
      diag_add_arg_type(d, default_expr_type);
      diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_default_expr));
      diag_flush(d);
      return NULL;
    }
  }

  PDeclParam* node = CREATE_DECL(PDeclParam, P_DECL_PARAM);
  P_DECL_GET_NAME(node) = p_name;
  P_DECL_GET_TYPE(node) = p_type;
  node->default_expr = p_default_expr;

  symbol = p_scope_add_symbol(p_s->current_scope, p_name);
  symbol->decl = (PDecl*)node;

  return node;
}

PDeclVar*
sema_act_on_var_decl(PSema* p_s, PSourceRange p_name_range, PIdentifierInfo* p_name, PType* p_type, PAst* p_init_expr)
{
  if (p_name == NULL)
    return NULL;

  PSymbol* symbol = sema_local_lookup(p_s, p_name);
  if (symbol != NULL) {
    PDiag* d = diag_at(P_DK_err_redeclaration_variable, p_name_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return NULL;
  }

  if (p_type == NULL) {
    if (p_init_expr == NULL) {
      PDiag* d = diag_at(P_DK_err_cannot_deduce_var_type, p_name_range.begin);
      diag_add_arg_ident(d, p_name);
      diag_add_source_range(d, p_name_range);
      diag_flush(d);
      return NULL;
    }

    p_type = p_ast_get_type(p_init_expr);
  } else if (p_init_expr != NULL) {
    PType* init_expr_type = p_ast_get_type(p_init_expr);
    if (!are_types_compatible(init_expr_type, p_type)) {
      PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_init_expr).begin);
      diag_add_arg_type(d, p_type);
      diag_add_arg_type(d, init_expr_type);
      diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_init_expr));
      diag_flush(d);
      return NULL;
    }
  }

  PDeclVar* node = CREATE_DECL(PDeclVar, P_DECL_VAR);
  P_DECL_GET_NAME(node) = p_name;
  P_DECL_GET_TYPE(node) = p_type;
  node->init_expr = p_init_expr;

  symbol = p_scope_add_symbol(p_s->current_scope, p_name);
  symbol->decl = (PDecl*)node;

  return node;
}

PAstLetStmt*
sema_act_on_let_stmt(PSema* p_s, PDeclVar** p_decls, size_t p_decl_count)
{
  assert(p_decl_count == 1);

  PAstLetStmt* node = CREATE_NODE(PAstLetStmt, P_AST_NODE_LET_STMT);
  node->var_decl = (PDecl*)p_decls[0];
  return node;
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

PAstBreakStmt*
sema_act_on_break_stmt(PSema* p_s, PSourceRange p_range)
{
  PScope* break_scope = find_nearest_scope_with_flag(p_s, P_SF_BREAK);
  if (break_scope == NULL) {
    PDiag* d = diag_at(P_DK_err_break_or_continue_outside_of_loop, p_range.begin);
    diag_add_arg_str(d, "break");
    diag_add_source_range(d, p_range);
    diag_flush(d);
    return NULL;
  }

  PAstBreakStmt* node = CREATE_NODE(PAstBreakStmt, P_AST_NODE_BREAK_STMT);
  node->loop_target = break_scope->statement;
  assert(node->loop_target != NULL);
  return node;
}

PAstContinueStmt*
sema_act_on_continue_stmt(PSema* p_s, PSourceRange p_range)
{
  PScope* continue_scope = find_nearest_scope_with_flag(p_s, P_SF_CONTINUE);
  if (continue_scope == NULL) {
    PDiag* d = diag_at(P_DK_err_break_or_continue_outside_of_loop, p_range.begin);
    diag_add_arg_str(d, "continue");
    diag_add_source_range(d, p_range);
    diag_flush(d);
    return NULL;
  }

  PAstContinueStmt* node = CREATE_NODE(PAstContinueStmt, P_AST_NODE_CONTINUE_STMT);
  node->loop_target = continue_scope->statement;
  assert(node->loop_target != NULL);
  return node;
}

PAstReturnStmt*
sema_act_on_return_stmt(PSema* p_s, PSourceLocation p_semi_loc, PAst* p_ret_expr)
{
  if (p_ret_expr != NULL) {
    PType* ret_type = p_ast_get_type(p_ret_expr);
    if (!are_types_compatible(ret_type, p_s->curr_func_type->ret_type)) {
      PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_ret_expr).begin);
      diag_add_arg_type(d, p_s->curr_func_type->ret_type);
      diag_add_arg_type(d, ret_type);
      diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_ret_expr));
      diag_flush(d);
      return NULL;
    }

    p_ret_expr = convert_to_rvalue(p_ret_expr);
  } else { // p_ret_expr == NULL
    if (!p_type_is_void(p_s->curr_func_type->ret_type)) {
      PDiag* d = diag_at(P_DK_err_expected_type, p_semi_loc);
      diag_add_arg_type(d, p_s->curr_func_type->ret_type);
      diag_add_arg_type(d, p_type_get_void());
      diag_flush(d);
      return NULL;
    }
  }

  PAstReturnStmt* node = CREATE_NODE(PAstReturnStmt, P_AST_NODE_RETURN_STMT);
  node->ret_expr = p_ret_expr;
  return node;
}

static void
check_suspicious_condition_expr(PAst* p_cond_expr)
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

static bool
check_condition_expr(PAst* p_cond_expr)
{
  check_suspicious_condition_expr(p_cond_expr);

  PType* cond_type = p_ast_get_type(p_cond_expr);
  if (p_type_is_bool(cond_type))
    return true;

  PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_cond_expr).begin);
  diag_add_arg_type(d, p_type_get_bool());
  diag_add_arg_type(d, cond_type);
  diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_cond_expr));
  diag_flush(d);
  return false;
}

PAstIfStmt*
sema_act_on_if_stmt(PSema* p_s, PAst* p_cond_expr, PAst* p_then_stmt, PAst* p_else_stmt)
{
  if (p_cond_expr == NULL || p_then_stmt == NULL)
    return NULL;

  if (!check_condition_expr(p_cond_expr))
    return NULL;

  p_cond_expr = convert_to_rvalue(p_cond_expr);

  PAstIfStmt* node = CREATE_NODE(PAstIfStmt, P_AST_NODE_IF_STMT);
  node->cond_expr = p_cond_expr;
  node->then_stmt = p_then_stmt;
  node->else_stmt = p_else_stmt;
  return node;
}

PAstLoopStmt*
sema_act_on_loop_stmt(PSema* p_s, PScope* p_scope)
{
  assert(p_scope != NULL);

  PAstLoopStmt* node = CREATE_NODE(PAstLoopStmt, P_AST_NODE_LOOP_STMT);
  p_scope->statement = (PAst*)node;
  return node;
}

PAstWhileStmt*
sema_act_on_while_stmt(PSema* p_s, PAst* p_cond_expr, PScope* p_scope)
{
  if (p_cond_expr == NULL)
    return NULL;

  assert(p_scope != NULL);

  if (!check_condition_expr(p_cond_expr))
    return NULL;

  p_cond_expr = convert_to_rvalue(p_cond_expr);

  PAstWhileStmt* node = CREATE_NODE(PAstWhileStmt, P_AST_NODE_WHILE_STMT);
  node->cond_expr = p_cond_expr;
  p_scope->statement = (PAst*)node;
  return node;
}

PAstBoolLiteral*
sema_act_on_bool_literal(PSema* p_s, PSourceRange p_range, bool p_value)
{
  PAstBoolLiteral* node = CREATE_NODE(PAstBoolLiteral, P_AST_NODE_BOOL_LITERAL);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_VC_RVALUE;
  node->value = p_value;
  return node;
}

PAstIntLiteral*
sema_act_on_int_literal(PSema* p_s, PSourceRange p_range, PType* p_type, uintmax_t p_value)
{
  if (p_type == NULL)
    p_type = p_type_get_i32();

  uintmax_t max_value = UINTMAX_MAX;
  switch (P_TYPE_GET_KIND(p_type)) {
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

  if (p_value > max_value) {
    PDiag* d = diag_at(P_DK_err_int_literal_too_large, p_range.begin);
    diag_add_arg_type(d, p_type);
    diag_add_source_range(d, p_range);
    diag_flush(d);
    p_value = 0; // set a value to recover
  }

  PAstIntLiteral* node = CREATE_NODE(PAstIntLiteral, P_AST_NODE_INT_LITERAL);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_VC_RVALUE;
  node->type = p_type;
  node->value = p_value;
  return node;
}

PAstFloatLiteral*
sema_act_on_float_literal(PSema* p_s, PSourceRange p_range, PType* p_type, double p_value)
{
  if (p_type == NULL)
    p_type = p_type_get_f32();

  PAstFloatLiteral* node = CREATE_NODE(PAstFloatLiteral, P_AST_NODE_FLOAT_LITERAL);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_VC_RVALUE;
  node->type = p_type;
  node->value = p_value;
  return node;
}

PAstDeclRefExpr*
sema_act_on_decl_ref_expr(PSema* p_s, PSourceRange p_range, PIdentifierInfo* p_name)
{
  if (p_name == NULL)
    return NULL;

  PSymbol* symbol = sema_lookup(p_s, p_name);
  if (symbol == NULL) {
    PDiag* d = diag_at(P_DK_err_use_undeclared_ident, p_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_range);
    diag_flush(d);
    return NULL;
  }

  assert(symbol->decl != NULL);
  symbol->decl->common.used = true;

  PAstDeclRefExpr* node = CREATE_NODE(PAstDeclRefExpr, P_AST_NODE_DECL_REF_EXPR);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_VC_LVALUE;
  node->decl = symbol->decl;
  return node;
}

PAstParenExpr*
sema_act_on_paren_expr(PSema* p_s, PSourceLocation p_lparen_loc, PSourceLocation p_rparen_loc, PAst* p_sub_expr)
{
  if (p_sub_expr == NULL)
    return NULL;

  PAstParenExpr* node = CREATE_NODE(PAstParenExpr, P_AST_NODE_PAREN_EXPR);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_AST_EXPR_GET_VALUE_CATEGORY(p_sub_expr);
  node->sub_expr = p_sub_expr;
  node->lparen_loc = p_lparen_loc;
  node->rparen_loc = p_rparen_loc;
  return node;
}

PAstBinaryExpr*
sema_act_on_binary_expr(PSema* p_s, PAstBinaryOp p_opcode, PSourceLocation p_op_loc, PAst* p_lhs, PAst* p_rhs)
{
  if (p_lhs == NULL || p_rhs == NULL)
    return NULL;

  PType* lhs_type = p_ast_get_type(p_lhs);
  PType* rhs_type = p_ast_get_type(p_rhs);

  PType* result_type = NULL;
  switch (p_opcode) {
    case P_BINARY_LOG_AND:
    case P_BINARY_LOG_OR: {
      bool has_error = false;

      if (!p_type_is_bool(lhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, p_op_loc);
        diag_add_arg_type(d, p_type_get_bool());
        diag_add_arg_type(d, lhs_type);
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_lhs));
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_rhs));
        diag_flush(d);
        has_error = true;
      }

      if (!p_type_is_bool(rhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, p_op_loc);
        diag_add_arg_type(d, p_type_get_bool());
        diag_add_arg_type(d, rhs_type);
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_lhs));
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_rhs));
        diag_flush(d);
        has_error = true;
      }

      if (has_error)
        return NULL;

      result_type = lhs_type;
    } break;

    case P_BINARY_EQ:
    case P_BINARY_NE:
    case P_BINARY_LT:
    case P_BINARY_LE:
    case P_BINARY_GT:
    case P_BINARY_GE:
      if (!are_types_compatible(lhs_type, rhs_type)) {
        PDiag* d = diag_at(P_DK_err_cannot_bin_op_generic, p_op_loc);
        diag_add_arg_str(d, p_binop_get_spelling(p_opcode));
        diag_add_arg_type(d, lhs_type);
        diag_add_arg_type(d, rhs_type);
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_lhs));
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_rhs));
        diag_flush(d);
        return NULL;
      }

      result_type = p_type_get_bool();
      break;

    case P_BINARY_BIT_AND:
    case P_BINARY_BIT_XOR:
    case P_BINARY_BIT_OR:
    case P_BINARY_ASSIGN_BIT_AND:
    case P_BINARY_ASSIGN_BIT_XOR:
    case P_BINARY_ASSIGN_BIT_OR: {
      bool has_error = false;

      if (!p_type_is_int(lhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, p_op_loc);
        diag_add_arg_type(d, p_type_get_generic_int());
        diag_add_arg_type(d, lhs_type);
        diag_flush(d);
        has_error = true;
      }

      if (!p_type_is_int(rhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, p_op_loc);
        diag_add_arg_type(d, p_type_get_generic_int());
        diag_add_arg_type(d, rhs_type);
        diag_flush(d);
        has_error = true;
      }

      if (has_error)
        return NULL;
    }
      HEDLEY_FALL_THROUGH;
    default:
      if (!are_types_compatible(lhs_type, rhs_type) || !p_type_is_arithmetic(lhs_type)) {
        PDiagKind diag_kind = P_DK_err_cannot_bin_op_generic;
        switch (p_opcode) {
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

        PDiag* d = diag_at(diag_kind, p_op_loc);
        if (diag_kind == P_DK_err_cannot_bin_op_generic) {
          const char* spelling = p_binop_get_spelling(p_opcode);
          diag_add_arg_str(d, spelling);
        }

        diag_add_arg_type(d, lhs_type);
        diag_add_arg_type(d, rhs_type);
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_lhs));
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_rhs));
        diag_flush(d);
        return NULL;
      }

      result_type = lhs_type;
      break;
  }

  assert(result_type != NULL);

  // Implicit lvalue->rvalue conversion
  p_rhs = convert_to_rvalue(p_rhs);
  PValueCategory result_vc;
  if (!p_binop_is_assignment(p_opcode)) {
    p_lhs = convert_to_rvalue(p_lhs);
    result_vc = P_VC_RVALUE;
  } else {
    result_vc = P_VC_LVALUE;
  }

  PAstBinaryExpr* node = CREATE_NODE(PAstBinaryExpr, P_AST_NODE_BINARY_EXPR);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = result_vc;
  node->type = result_type;
  node->lhs = p_lhs;
  node->rhs = p_rhs;
  node->opcode = p_opcode;
  node->op_loc = p_op_loc;
  return node;
}

/* If p_callee is a decl ref expr then returns its name
 * otherwise returns NULL. */
static PIdentifierInfo*
get_decl_ref_name(PAst* p_callee)
{
  p_callee = p_ast_ignore_parens(p_callee);
  if (P_AST_GET_KIND(p_callee) == P_AST_NODE_DECL_REF_EXPR) {
    PIdentifierInfo* name = P_DECL_GET_NAME(((PAstDeclRefExpr*)(p_callee))->decl);
    return name;
  }

  return NULL;
}

static bool
check_call_args(PSourceRange p_call_range,
                PSourceLocation p_lparen_loc,
                PAst* p_callee,
                PFunctionType* p_callee_type,
                PAst** p_args,
                size_t p_arg_count)
{
  if (p_arg_count < p_callee_type->arg_count) {
    PDiag* d = diag_at(P_DK_err_too_few_args, p_lparen_loc);
    PIdentifierInfo* callee_name = get_decl_ref_name(p_callee);
    if (callee_name != NULL)
      diag_add_arg_type_with_name_hint(d, (PType*)p_callee_type, callee_name);
    else
      diag_add_arg_type(d, (PType*)p_callee_type);
    diag_add_source_range(d, p_call_range);
    diag_flush(d);
    return false;
  } else if (p_arg_count > p_callee_type->arg_count) {
    PDiag* d = diag_at(P_DK_err_too_many_args, p_lparen_loc);
    PIdentifierInfo* callee_name = get_decl_ref_name(p_callee);
    if (callee_name != NULL)
      diag_add_arg_type_with_name_hint(d, (PType*)p_callee_type, callee_name);
    else
      diag_add_arg_type(d, (PType*)p_callee_type);
    diag_add_source_range(d, p_call_range);
    diag_flush(d);
    return false;
  }

  bool has_error = false;
  PType** args_expected_type = p_callee_type->args;
  for (size_t i = 0; i < p_arg_count; ++i) {
    if (p_args[i] == NULL) {
      has_error = true;
      continue;
    }

    PType* arg_type = p_ast_get_type(p_args[i]);
    if (!are_types_compatible(arg_type, args_expected_type[i])) {
      PDiag* d = diag_at(P_DK_err_expected_type, P_AST_GET_SOURCE_RANGE(p_args[i]).begin);
      diag_add_arg_type(d, args_expected_type[i]);
      diag_add_arg_type(d, arg_type);
      diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_args[i]));
      diag_flush(d);
      has_error = true;
    }

    p_args[i] = convert_to_rvalue(p_args[i]);
  }

  return !has_error;
}

PAstCallExpr*
sema_act_on_call_expr(PSema* p_s,
                      PSourceLocation p_lparen_loc,
                      PSourceLocation p_rparen_loc,
                      PAst* p_callee,
                      PAst** p_args,
                      size_t p_arg_count)
{
  if (p_callee == NULL || p_args == NULL)
    return NULL;

  PSourceRange call_range = { P_AST_GET_SOURCE_RANGE(p_callee).begin, p_rparen_loc };

  PType* callee_type = p_ast_get_type(p_callee);
  if (!p_type_is_function(callee_type)) {
    PDiag* d;

    PIdentifierInfo* callee_name = get_decl_ref_name(p_callee);
    if (callee_name != NULL) {
      d = diag_at(P_DK_err_cannot_be_used_as_function, p_lparen_loc);
      diag_add_arg_ident(d, callee_name);
    } else {
      d = diag_at(P_DK_err_expr_cannot_be_used_as_function, p_lparen_loc);
    }

    diag_add_source_range(d, call_range);
    diag_flush(d);
    return NULL;
  }

  if (!check_call_args(call_range, p_lparen_loc, p_callee, (PFunctionType*)callee_type, p_args, p_arg_count))
    return NULL;

  PAstCallExpr* node = CREATE_NODE_EXTRA_SIZE(PAstCallExpr, sizeof(PAst*) * (p_arg_count - 1), P_AST_NODE_CALL_EXPR);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_VC_RVALUE;
  node->callee = p_callee;
  node->arg_count = p_arg_count;
  memcpy(node->args, p_args, sizeof(PAst*) * p_arg_count);
  return node;
}

PAstMemberExpr*
sema_act_on_member_expr(PSema* p_s,
                        PSourceLocation p_dot_loc,
                        PAst* p_base_expr,
                        PSourceRange p_name_range,
                        PIdentifierInfo* p_name)
{
  if (p_base_expr == NULL || p_name == NULL)
    return NULL;

  PType* base_type = p_ast_get_type(p_base_expr);
  if (P_TYPE_GET_KIND(base_type) != P_TYPE_TAG || P_DECL_GET_KIND(((PTagType*)base_type)->decl) != P_DECL_STRUCT) {
    PDiag* d = diag_at(P_DK_err_member_not_struct, p_dot_loc);
    diag_add_arg_ident(d, p_name);

    PSourceRange dot_range = { p_dot_loc, p_dot_loc };
    diag_add_source_range(d, dot_range);
    diag_flush(d);
    return NULL;
  }

  PDeclStructField* field_decl = NULL;
  PDeclStruct* base_decl = (PDeclStruct*)((PTagType*)base_type)->decl;
  for (int i = 0; i < base_decl->field_count; ++i) {
    if (P_DECL_GET_NAME(base_decl->fields[i]) == p_name) {
      field_decl = base_decl->fields[i];
      break;
    }
  }

  if (field_decl == NULL) {
    PDiag* d = diag_at(P_DK_err_no_member_named, p_dot_loc);
    diag_add_arg_type(d, base_type);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return NULL;
  }

  PAstMemberExpr* node = CREATE_NODE(PAstMemberExpr, P_AST_NODE_MEMBER_EXPR);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_VC_LVALUE;
  node->base_expr = p_base_expr;
  node->member = field_decl;
  return node;
}

PAstUnaryExpr*
sema_act_on_unary_expr(PSema* p_s, PAstUnaryOp p_opcode, PSourceLocation p_op_loc, PAst* p_sub_expr)
{
  if (p_sub_expr == NULL)
    return NULL;

  PType* result_type = p_ast_get_type(p_sub_expr);
  PValueCategory result_vc;
  switch (p_opcode) {
    case P_UNARY_NEG: // '-' operator
      if (!p_type_is_float(result_type) && !p_type_is_signed(result_type)) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_unary_op, p_op_loc);
        diag_add_arg_char(d, '-');
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_sub_expr));
        diag_flush(d);
        return NULL;
      }

      p_sub_expr = convert_to_rvalue(p_sub_expr);
      result_vc = P_VC_RVALUE;
      break;
    case P_UNARY_NOT: // logical and bitwise '!' operator
      if (!p_type_is_bool(result_type) && !p_type_is_int(result_type)) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_unary_op, p_op_loc);
        diag_add_arg_char(d, '!');
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_sub_expr));
        diag_flush(d);
        return NULL;
      }

      p_sub_expr = convert_to_rvalue(p_sub_expr);
      result_vc = P_VC_RVALUE;
      break;
    case P_UNARY_ADDRESS_OF: // '&' operator
      if (P_AST_EXPR_IS_RVALUE(p_sub_expr)) {
        PDiag* d = diag_at(P_DK_err_could_not_take_addr_rvalue, p_op_loc);
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_sub_expr));
        diag_flush(d);
        return NULL;
      }

      result_type = p_type_get_pointer(result_type);
      result_vc = P_VC_RVALUE;
      break;
    case P_UNARY_DEREF: // '*' operator
      if (!p_type_is_pointer(result_type)) {
        PDiag* d = diag_at(P_DK_err_indirection_requires_ptr, p_op_loc);
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, P_AST_GET_SOURCE_RANGE(p_sub_expr));
        diag_flush(d);
        return NULL;
      }

      result_type = ((PPointerType*)result_type)->element_type;
      result_vc = P_VC_LVALUE;
      break;
    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }

  PAstUnaryExpr* node = CREATE_NODE(PAstUnaryExpr, P_AST_NODE_UNARY_EXPR);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = result_vc;
  node->sub_expr = p_sub_expr;
  node->opcode = p_opcode;
  node->op_loc = p_op_loc;
  node->type = result_type;
  return node;
}

/* Classify {int} -> p_to_type cast. */
static PAstCastKind
classify_int_cast(PType* p_from_type, PType* p_to_type)
{
  if (p_type_is_float(p_to_type))
    return P_CAST_INT2FLOAT;

  if (p_type_is_int(p_to_type)) {
    int from_bitwidth = p_type_get_bitwidth(p_from_type);
    int to_bitwidth = p_type_get_bitwidth(p_to_type);
    if (from_bitwidth != to_bitwidth) {
      return P_CAST_INT2INT;
    }

    // Signed <-> Unsigned integer conversion.
    return P_CAST_NOOP;
  }

  return P_CAST_INVALID;
}

/* Classify {float} -> p_to_type cast. */
static PAstCastKind
classify_float_cast(PType* p_to_type)
{
  if (p_type_is_int(p_to_type))
    return P_CAST_FLOAT2INT;

  if (p_type_is_float(p_to_type))
    return P_CAST_FLOAT2FLOAT;

  return P_CAST_INVALID;
}

/* Classify bool -> p_to_type cast. */
static PAstCastKind
classify_bool_cast(PType* p_to_type)
{
  if (p_type_is_int(p_to_type))
    return P_CAST_BOOL2INT;

  if (p_type_is_float(p_to_type))
    return P_CAST_BOOL2FLOAT;

  return P_CAST_INVALID;
}

PAstCastExpr*
sema_act_on_cast_expr(PSema* p_s, PSourceLocation p_as_loc, PType* p_to_type, PAst* p_sub_expr)
{
  if (p_to_type == NULL || p_sub_expr == NULL)
    return NULL;

  PType* from_type = p_ast_get_type(p_sub_expr);

  PAstCastKind cast_kind = P_CAST_INVALID;
  if (p_type_get_canonical(from_type) == p_type_get_canonical(p_to_type))
    cast_kind = P_CAST_NOOP; // from/to types are equivalent
  else if (p_type_is_int(from_type))
    cast_kind = classify_int_cast(from_type, p_to_type);
  else if (p_type_is_int(from_type))
    cast_kind = classify_float_cast(p_to_type);
  else if (p_type_is_int(from_type))
    cast_kind = classify_bool_cast(p_to_type);

  // TODO: issue a warning when cast is noop

  if (cast_kind == P_CAST_INVALID) {
    PDiag* d = diag_at(P_DK_err_unsupported_conversion, p_as_loc);
    diag_add_arg_type(d, from_type);
    diag_add_arg_type(d, p_to_type);
    diag_flush(d);
    return NULL;
  }

  PAstCastExpr* node = CREATE_NODE(PAstCastExpr, P_AST_NODE_CAST_EXPR);
  P_AST_EXPR_GET_VALUE_CATEGORY(node) = P_AST_EXPR_GET_VALUE_CATEGORY(p_sub_expr);
  node->sub_expr = p_sub_expr;
  node->type = p_to_type;
  node->cast_kind = cast_kind;
  node->as_loc = p_as_loc;
  return node;
}
