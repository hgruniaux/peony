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

PDeclFunction*
sema_act_on_func_decl(PSema* p_s,
                      PSourceRange p_name_range,
                      PIdentifierInfo* p_name,
                      PType* p_ret_type,
                      PDeclParam** p_params,
                      size_t p_param_count);

void
sema_begin_func_decl_body_parsing(PSema* p_s, PDeclFunction* p_decl);

void
sema_end_func_decl_body_parsing(PSema* p_s, PDeclFunction* p_decl);

PDeclParam*
sema_act_on_param_decl(PSema* p_s,
                       PSourceRange p_name_range,
                       PIdentifierInfo* p_name,
                       PType* p_type,
                       PAst* p_default_expr);

PDeclVar*
sema_act_on_var_decl(PSema* p_s, PSourceRange p_name_range, PIdentifierInfo* p_name, PType* p_type, PAst* p_init_expr);

PAstLetStmt*
sema_act_on_let_stmt(PSema* p_s, PDeclVar** p_decls, size_t p_decl_count);

PAstBreakStmt*
sema_act_on_break_stmt(PSema* p_s, PSourceRange p_range);

PAstContinueStmt*
sema_act_on_continue_stmt(PSema* p_s, PSourceRange p_range);

PAstReturnStmt*
sema_act_on_return_stmt(PSema* p_s, PSourceLocation p_semi_loc, PAst* p_ret_expr);

PAstIfStmt*
sema_act_on_if_stmt(PSema* p_s, PAst* p_cond_expr, PAst* p_then_stmt, PAst* p_else_stmt);

PAstLoopStmt*
sema_act_on_loop_stmt(PSema* p_s, PScope* p_scope);

PAstWhileStmt*
sema_act_on_while_stmt(PSema* p_s, PAst* p_cond_expr, PScope* p_scope);

PAstBoolLiteral*
sema_act_on_bool_literal(PSema* p_s, PSourceRange p_range, bool p_value);

PAstIntLiteral*
sema_act_on_int_literal(PSema* p_s, PSourceRange p_range, PType* p_type, uintmax_t p_value);

PAstFloatLiteral*
sema_act_on_float_literal(PSema* p_s, PSourceRange p_range, PType* p_type, double p_value);

PAstDeclRefExpr*
sema_act_on_decl_ref_expr(PSema* p_s, PSourceRange p_range, PIdentifierInfo* p_name);

PAstParenExpr*
sema_act_on_paren_expr(PSema* p_s, PSourceLocation p_lparen_loc, PSourceLocation p_rparen_loc, PAst* p_sub_expr);

PAstCallExpr*
sema_act_on_call_expr(PSema* p_s,
                      PSourceLocation p_lparen_loc,
                      PSourceLocation p_rparen_loc,
                      PAst* p_callee,
                      PAst** p_args,
                      size_t p_arg_count);

PAstBinaryExpr*
sema_act_on_binary_expr(PSema* p_s, PAstBinaryOp p_opcode, PSourceLocation p_op_loc, PAst* p_lhs, PAst* p_rhs);

PAstUnaryExpr*
sema_act_on_unary_expr(PSema* p_s, PAstUnaryOp p_opcode, PSourceLocation p_op_loc, PAst* p_sub_expr);

PAstCastExpr*
sema_act_on_cast_expr(PSema* p_s, PSourceLocation p_as_loc, PType* p_to_type, PAst* p_sub_expr);
