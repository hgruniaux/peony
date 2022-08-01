#pragma once

#include "identifier_table.h"
#include "type.h"
#include "utils/source_location.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum PAstKind
{
  P_AST_NODE_TRANSLATION_UNIT,
  P_AST_NODE_COMPOUND_STMT,
  P_AST_NODE_LET_STMT,
  P_AST_NODE_BREAK_STMT,
  P_AST_NODE_CONTINUE_STMT,
  P_AST_NODE_RETURN_STMT,
  P_AST_NODE_WHILE_STMT,
  P_AST_NODE_IF_STMT,
  P_AST_NODE_BOOL_LITERAL,
  P_AST_NODE_INT_LITERAL,
  P_AST_NODE_FLOAT_LITERAL,
  P_AST_NODE_PAREN_EXPR,
  P_AST_NODE_DECL_REF_EXPR,
  P_AST_NODE_UNARY_EXPR,
  P_AST_NODE_BINARY_EXPR,
  P_AST_NODE_CALL_EXPR,
  P_AST_NODE_CAST_EXPR,
  P_AST_NODE_LVALUE_TO_RVALUE_EXPR
} PAstKind;

typedef struct PDecl PDecl;

/** @brief A value category (either lvalue or rvalue). */
typedef enum PValueCategory
{
  P_VC_LVALUE,
  P_VC_RVALUE
} PValueCategory;

typedef struct PAstCommon
{
  PAstKind kind;
  PSourceRange source_range;
} PAstCommon;

typedef struct PAstExprCommon
{
  PType* type;
  PValueCategory value_category;
} PAstExprCommon;

typedef struct PAst
{
  PAstCommon common;
} PAst;

typedef struct PAstTranslationUnit
{
  PAstCommon common;
  int decl_count;
  PDecl* decls[1]; /* tail-allocated */
} PAstTranslationUnit;

/** @brief A `{ stmt... }` statement. */
typedef struct PAstCompoundStmt
{
  PAstCommon common;
  PSourceLocation lbrace_loc;
  PSourceLocation rbrace_loc;
  int stmt_count;
  PAst* stmts[1]; /* tail-allocated */
} PAstCompoundStmt;

/** @brief A `let name = init_expr;` statement. */
typedef struct PAstLetStmt
{
  PAstCommon common;
  PDecl* var_decl;
} PAstLetStmt;

/** @brief A `break;` statement. */
typedef struct PAstBreakStmt
{
  PAstCommon common;
  PAst* loop_target; /* an instance of PAstWhileStmt, etc. */
} PAstBreakStmt;

/** @brief A `continue;` statement. */
typedef struct PAstContinueStmt
{
  PAstCommon common;
  PAst* loop_target; /* an instance of PAstWhileStmt, etc. */
} PAstContinueStmt;

/** @brief A `return ret_expr;` statement. */
typedef struct PAstReturnStmt
{
  PAstCommon common;
  PAst* ret_expr;
} PAstReturnStmt;

/** @brief A `if (cond_expr) then_stmt else else_stmt`. */
typedef struct PAstIfStmt
{
  PAstCommon common;
  PAst* cond_expr;
  PAst* then_stmt;
  PAst* else_stmt;
} PAstIfStmt;

typedef struct PAstLoopCommon
{
  void* _llvm_break_label;
  void* _llvm_continue_label;
} PAstLoopCommon;

typedef struct PAstLoopStmt
{
  PAstCommon common;
  PAstLoopCommon loop_common;
} PAstLoopStmt;

/** @brief A `while (cond_expr) body_stmt`. */
typedef struct PAstWhileStmt
{
  PAstCommon common;
  PAstLoopCommon loop_common;
  PAst* cond_expr;
  PAst* body_stmt;
} PAstWhileStmt;

typedef struct PAstExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
} PAstExpr;

/** @brief A bool literal (e.g. `true` or `false`). */
typedef struct PAstBoolLiteral
{
  PAstCommon common;
  PAstExprCommon expr_common;
  bool value;
} PAstBoolLiteral;

/** @brief An integer literal (e.g. `42`). */
typedef struct PAstIntLiteral
{
  PAstCommon common;
  PAstExprCommon expr_common;
  uintmax_t value;
} PAstIntLiteral;

/** @brief A float literal (e.g. `3.14`). */
typedef struct PAstFloatLiteral
{
  PAstCommon common;
  PAstExprCommon expr_common;
  double value;
} PAstFloatLiteral;

/** @brief A parenthesized expression (e.g. `(sub_expr)`). */
typedef struct PAstParenExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PSourceLocation lparen_loc;
  PSourceLocation rparen_loc;
  PAst* sub_expr;
} PAstParenExpr;

/** @brief A declaration reference (e.g. `x` where x is a variable). */
typedef struct PAstDeclRefExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PDecl* decl;
} PAstDeclRefExpr;

/** @brief Supported unary operators. */
typedef enum PAstUnaryOp
{
#define UNARY_OPERATOR(p_kind) p_kind,
#include "operator_kinds.def"
} PAstUnaryOp;

/** @brief An unary expression (e.g. `-x`). */
typedef struct PAstUnaryExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PSourceLocation op_loc;
  PAstUnaryOp opcode;
  PAst* sub_expr;
} PAstUnaryExpr;

/** @brief Supported binary operators. */
typedef enum PAstBinaryOp
{
#define BINARY_OPERATOR(p_kind, p_tok, p_precedence) p_kind,
#include "operator_kinds.def"
} PAstBinaryOp;

bool
p_binop_is_assignment(PAstBinaryOp p_binop);

/** @brief A binary expression (e.g. `a + b`). */
typedef struct PAstBinaryExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PSourceLocation op_loc;
  PAstBinaryOp opcode;
  PAst* lhs;
  PAst* rhs;
} PAstBinaryExpr;

/** @brief A function call. */
typedef struct PAstCallExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PAst* callee;
  int arg_count;
  PAst* args[1]; /* tail-allocated */
} PAstCallExpr;

/** @brief Supported cast kinds. */
typedef enum PAstCastKind
{
#define CAST_OPERATOR(p_kind) p_kind,
#include "operator_kinds.def"
} PAstCastKind;

/** @brief A cast expression (e.g. `sub_expr as i32`). */
typedef struct PAstCastExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PAst* sub_expr;
  PAstCastKind cast_kind;
} PAstCastExpr;

/** @brief An implicit lvalue -> rvalue cast generated by the semantic analyzer. */
typedef struct PAstLValueToRValueExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PAst* sub_expr;
} PAstLValueToRValueExpr;

#define P_AST_GET_KIND(p_node) (((PAst*)(p_node))->common.kind)
#define P_AST_GET_SOURCE_RANGE(p_node) (((PAst*)(p_node))->common.source_range)
#define P_AST_EXPR_GET_TYPE(p_node) (((PAstExpr*)(p_node))->expr_common.type)
#define P_AST_EXPR_GET_VALUE_CATEGORY(p_node) (((PAstExpr*)(p_node))->expr_common.value_category)
#define P_AST_EXPR_IS_LVALUE(p_node) (P_AST_EXPR_GET_VALUE_CATEGORY(p_node) == P_VC_LVALUE)
#define P_AST_EXPR_IS_RVALUE(p_node) (P_AST_EXPR_GET_VALUE_CATEGORY(p_node) == P_VC_RVALUE)

typedef enum PDeclKind
{
  P_DECL_FUNCTION,
  P_DECL_VAR,
  P_DECL_PARAM
} PDeclKind;

typedef struct PDeclCommon
{
  PDeclKind kind;
  PIdentifierInfo* name;
  PType* type;
  void* _llvm_address; /* used by the LLVM backend */
  bool used;
} PDeclCommon;

typedef struct PDecl
{
  PDeclCommon common;
} PDecl;

/** @brief A variable declaration. */
typedef struct PDeclVar
{
  PDeclCommon common;
  PAst* init_expr;
} PDeclVar;

/** @brief A function parameter declaration.
 *
 * Must keep the same layout as PDeclVar. */
typedef struct PDeclParam
{
  PDeclCommon common;
  PAst* default_expr;
} PDeclParam;

/** @brief A function declaration. */
typedef struct PDeclFunction
{
  PDeclCommon common;
  PAst* body;
  int param_count;
  PDeclParam* params[1]; /* tail-allocated */
} PDeclFunction;

#define P_DECL_GET_KIND(p_node) (((PDecl*)(p_node))->common.kind)
#define P_DECL_GET_NAME(p_node) (((PDecl*)(p_node))->common.name)
#define P_DECL_GET_TYPE(p_node) (((PDecl*)(p_node))->common.type)

PAst*
p_create_node_impl(size_t p_size, PAstKind p_kind);

#define CREATE_NODE(p_type, p_kind) ((p_type*)p_create_node_impl(sizeof(p_type), (p_kind)))
#define CREATE_NODE_EXTRA_SIZE(p_type, p_extra_size, p_kind)                                                           \
  ((p_type*)p_create_node_impl(sizeof(p_type) + (p_extra_size), (p_kind)))

PDecl*
p_create_decl_impl(size_t p_size, PDeclKind p_kind);

#define CREATE_DECL(p_type, p_kind) ((p_type*)p_create_decl_impl(sizeof(p_type), (p_kind)))
#define CREATE_DECL_EXTRA_SIZE(p_type, p_extra_size, p_kind)                                                           \
  ((p_type*)p_create_decl_impl(sizeof(p_type) + (p_extra_size), (p_kind)))
