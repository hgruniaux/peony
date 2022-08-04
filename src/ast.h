#pragma once

#include "identifier_table.h"
#include "type.h"
#include "utils/dynamic_array.h"
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
  P_AST_NODE_LOOP_STMT,
  P_AST_NODE_WHILE_STMT,
  P_AST_NODE_IF_STMT,
  P_AST_NODE_BOOL_LITERAL,
  P_AST_NODE_INT_LITERAL,
  P_AST_NODE_FLOAT_LITERAL,
  P_AST_NODE_PAREN_EXPR,
  P_AST_NODE_DECL_REF_EXPR,
  P_AST_NODE_UNARY_EXPR,
  P_AST_NODE_BINARY_EXPR,
  P_AST_NODE_MEMBER_EXPR,
  P_AST_NODE_CALL_EXPR,
  P_AST_NODE_CAST_EXPR,
  P_AST_NODE_LVALUE_TO_RVALUE_EXPR
} PAstKind;

typedef struct PDecl PDecl;

/* --------------------------------------------------------
 * Statements
 */

typedef struct PAstCommon
{
  PAstKind kind;
  PSourceRange source_range;
} PAstCommon;

typedef struct PAst
{
  PAstCommon common;
} PAst;

typedef struct PAstTranslationUnit
{
  PAstCommon common;
  size_t decl_count;
  PDecl* decls[1]; /* tail-allocated */
} PAstTranslationUnit;

/** @brief A `{ stmt... }` statement. */
typedef struct PAstCompoundStmt
{
  PAstCommon common;
  PSourceLocation lbrace_loc;
  PSourceLocation rbrace_loc;
  size_t stmt_count;
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

/** @brief A `if cond_expr { then_stmt } else { else_stmt }` (else_stmt is optional). */
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

/** @brief A `loop { body_stmt }`. */
typedef struct PAstLoopStmt
{
  PAstCommon common;
  PAstLoopCommon loop_common;
  PAst* body_stmt;
} PAstLoopStmt;

/** @brief A `while cond_expr { body_stmt }`. */
typedef struct PAstWhileStmt
{
  PAstCommon common;
  PAstLoopCommon loop_common;
  PAst* cond_expr;
  PAst* body_stmt;
} PAstWhileStmt;

/* --------------------------------------------------------
 * Expressions
 */

/** @brief A value category (either lvalue or rvalue). */
typedef enum PValueCategory
{
  P_VC_LVALUE,
  P_VC_RVALUE
} PValueCategory;

typedef struct PAstExprCommon
{
  PValueCategory value_category;
} PAstExprCommon;

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
  PType* type;
  uintmax_t value;
} PAstIntLiteral;

/** @brief A float literal (e.g. `3.14`). */
typedef struct PAstFloatLiteral
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PType* type;
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
  PType* type;
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

const char*
p_binop_get_spelling(PAstBinaryOp p_binop);

/** @brief A binary expression (e.g. `a + b`). */
typedef struct PAstBinaryExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PType* type;
  PSourceLocation op_loc;
  PAstBinaryOp opcode;
  PAst* lhs;
  PAst* rhs;
} PAstBinaryExpr;

struct PDeclStructField;

/** @brief A member access expression (e.g. `base_expr.member`). */
typedef struct PAstMemberExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PAst* base_expr;
  struct PDeclStructField* member;
} PAstMemberExpr;

/** @brief A function call. */
typedef struct PAstCallExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PAst* callee;
  size_t arg_count;
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
  PType* type;
  PAst* sub_expr;
  PAstCastKind cast_kind;
  PSourceLocation as_loc;
} PAstCastExpr;

/** @brief An implicit lvalue -> rvalue cast generated by the semantic analyzer. */
typedef struct PAstLValueToRValueExpr
{
  PAstCommon common;
  PAstExprCommon expr_common;
  PAst* sub_expr;
} PAstLValueToRValueExpr;

PType*
p_ast_get_type(PAst* p_ast);

PAst*
p_ast_ignore_parens(PAst* p_ast);

#define P_AST_GET_KIND(p_node) (((PAst*)(p_node))->common.kind)
#define P_AST_GET_SOURCE_RANGE(p_node) (((PAst*)(p_node))->common.source_range)
#define P_AST_EXPR_GET_VALUE_CATEGORY(p_node) (((PAstExpr*)(p_node))->expr_common.value_category)
#define P_AST_EXPR_IS_LVALUE(p_node) (P_AST_EXPR_GET_VALUE_CATEGORY(p_node) == P_VC_LVALUE)
#define P_AST_EXPR_IS_RVALUE(p_node) (P_AST_EXPR_GET_VALUE_CATEGORY(p_node) == P_VC_RVALUE)

/* --------------------------------------------------------
 * Declarations
 */

typedef enum PDeclKind
{
  P_DECL_FUNCTION,
  P_DECL_VAR,
  P_DECL_PARAM,
  P_DECL_STRUCT_FIELD,
  P_DECL_STRUCT
} PDeclKind;

typedef struct PDeclCommon
{
  PDeclKind kind;
  // Some declarations may be unnamed, in that case this is NULL.
  PIdentifierInfo* name;
  PType* type;
  // Private field used by the LLVM codegen backend to store the address
  // of the declaration instance (for variables and instances).
  // This is a LLVMValueRef (e.g. returned by LLVMBuildAlloca()).
  void* _llvm_address;
  // Is there at least one reference to this type?
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
  // Function bodies are parsed once all declarations were parsed and not
  // at the same time as when parsing the function prototype. When the
  // function prototype is parsed, the parser consumes all tokens that may
  // compose the function body (between and including the '{' and '}') and
  // stores them in lazy_body_token_run. Later the tokens from lazy_body_token_run
  // are parsed and lazy_body_token_run is cleared. Therefore, the body
  // is considered not parsed until lazy_body_token_run is empty.
  PDynamicArray lazy_body_token_run;
  // The count of parameters that do not have a default argument.
  size_t required_param_count;
  // The count of parameters of this function. It is also the size
  // of the params array.
  size_t param_count;
  PDeclParam* params[1]; // tail-allocated
} PDeclFunction;

/** @brief A structure field declaration. */
typedef struct PDeclStructField
{
  PDeclCommon common;
  // To whom does this field belong.
  struct PDeclStruct* parent;
  // This is the position in parent->fields where this field belong in.
  int idx_in_parent_fields;
} PDeclStructField;

/** @brief A structure declaration. */
typedef struct PDeclStruct
{
  PDeclCommon common;
  int field_count;
  PDeclStructField* fields[1]; // tail-allocated
} PDeclStruct;

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
