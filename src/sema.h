#pragma once

#include "ast.h"
#include "scope.h"

/*
 * This file contains the interface of the semantic analyzer.
 *
 * These functions are responsible for completing the nodes of
 * the AST with semantic information and checking whether this
 * information is valid according to the context. Possibly
 * emitting errors if it is not the case.
 *
 * This interface is used by the parser during the AST construction.
 */

#define P_MAX_SCOPE_CACHE 64

typedef struct PSema
{
  PFunctionType* curr_func_type;

  size_t current_scope_cache_idx;
  PScope* scope_cache[P_MAX_SCOPE_CACHE];
  PScope* current_scope;
} PSema;

void
sema_push_scope(PSema* p_s, PScopeFlags p_flags);

void
sema_pop_scope(PSema* p_s);

PSymbol*
sema_local_lookup(PSema* p_s, PIdentifierInfo* p_name);

PSymbol*
sema_lookup(PSema* p_s, PIdentifierInfo* p_name);

bool
sema_check_pre_func_decl(PSema* p_s, PDeclFunction* p_node, PType* return_type);

bool
sema_check_func_decl(PSema* p_s, PDeclFunction* p_node);

bool
sema_check_param_decl(PSema* p_s, PDeclParam* p_node);

bool
sema_check_break_stmt(PSema* p_s, PAstBreakStmt* p_node);

bool
sema_check_continue_stmt(PSema* p_s, PAstContinueStmt* p_node);

bool
sema_check_return_stmt(PSema* p_s, PAstReturnStmt* p_node);

bool
sema_check_if_stmt(PSema* p_s, PAstIfStmt* p_node);

bool
sema_check_while_stmt(PSema* p_s, PAstWhileStmt* p_node);

bool
sema_check_bool_literal(PSema* p_s, PAstBoolLiteral* p_node);

bool
sema_check_int_literal(PSema* p_s, PAstIntLiteral* p_node);

bool
sema_check_float_literal(PSema* p_s, PAstFloatLiteral* p_node);

bool
sema_check_decl_ref_expr(PSema* p_s, PAstDeclRefExpr* p_node, PIdentifierInfo* p_name);

bool
sema_check_paren_expr(PSema* p_s, PAstParenExpr* p_node);

bool
sema_check_unary_expr(PSema* p_s, PAstUnaryExpr* p_node);

bool
sema_check_binary_expr(PSema* p_s, PAstBinaryExpr* p_node);

bool
sema_check_call_expr(PSema* p_s, PAstCallExpr* p_node);

bool
sema_check_cast_expr(PSema* p_s, PAstCastExpr* p_node);
