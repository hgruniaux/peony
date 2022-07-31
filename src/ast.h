#pragma once

#include "identifier_table.h"
#include "type.h"

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
  P_AST_NODE_BINARY_EXPR,
  P_AST_NODE_CALL_EXPR
} PAstKind;

typedef struct PDecl PDecl;

typedef struct PAstCommon
{
  PAstKind kind;
} PAstCommon;

typedef struct PAstExprCommon
{
  PType* type;
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
} PAstBreakStmt;

/** @brief A `continue;` statement. */
typedef struct PAstContinueStmt
{
  PAstCommon common;
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

/** @brief A `while (cond_expr) body_stmt`. */
typedef struct PAstWhileStmt
{
  PAstCommon common;
  PAst* cond_expr;
  PAst* body_stmt;
} PAstWhileStmt;

/** @brief A bool literal (e.g. `true` or `false`). */
typedef struct PAstBoolLiteral
{
  PAstCommon common;
  PAstExprCommon expr;
  bool value;
} PAstBoolLiteral;

/** @brief An integer literal (e.g. `42`). */
typedef struct PAstIntLiteral
{
  PAstCommon common;
  PAstExprCommon expr;
  uintmax_t value;
} PAstIntLiteral;

/** @brief A float literal (e.g. `3.14`). */
typedef struct PAstFloatLiteral
{
  PAstCommon common;
  PAstExprCommon expr;
  double value;
} PAstFloatLiteral;

/** @brief A parenthesized expression (e.g. `(sub_expr)`). */
typedef struct PAstParenExpr
{
  PAstCommon common;
  PAstExprCommon expr;
  PAst* sub_expr;
} PAstParenExpr;

/** @brief A declaration reference (e.g. `x` where x is a variable). */
typedef struct PAstDeclRefExpr
{
  PAstCommon common;
  PAstExprCommon expr;
  PDecl* decl;
} PAstDeclRefExpr;

/** @brief A function call. */
typedef struct PAstCallExpr
{
  PAstCommon common;
  PAstExprCommon expr;
  PDecl* func;
  int arg_count;
  PAst* args[1]; /* tail-allocated */
} PAstCallExpr;

/** @brief Supported binary operators. */
typedef enum PAstBinaryOp
{
#define BINOP(p_kind, p_tok, p_precedence) p_kind,
#include "operator_kinds.def"
} PAstBinaryOp;

/** @brief A binary expression (e.g. `a + b`). */
typedef struct PAstBinaryExpr
{
  PAstCommon common;
  PAstExprCommon expr;
  PAstBinaryOp opcode;
  PAst* lhs;
  PAst* rhs;
} PAstBinaryExpr;

#define P_AST_GET_KIND(p_node) (((PAst*)(p_node))->common.kind)
#define P_AST_GET_TYPE(p_node) (((PAstParenExpr*)(p_node))->expr.type)

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
