#include "parser.h"
#include "scope.h"

#include "utils/diag.h"
#include "utils/dynamic_array.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define LOOKAHEAD_IS(p_kind) ((p_parser)->lookahead.kind == (p_kind))

static PAst*
create_node_impl(struct PParser* p_parser, size_t p_size, PAstKind p_kind)
{
  PAst* node = p_bump_alloc(&p_global_bump_allocator, p_size, P_ALIGNOF(PAst));
  assert(node != NULL);
  P_AST_GET_KIND(node) = p_kind;
  return node;
}

#define CREATE_NODE(p_type, p_kind) ((p_type*)create_node_impl(p_parser, sizeof(p_type), (p_kind)))
#define CREATE_NODE_EXTRA_SIZE(p_type, p_extra_size, p_kind)                                                           \
  ((p_type*)create_node_impl(p_parser, sizeof(p_type) + (p_extra_size), (p_kind)))

static PDecl*
create_decl_impl(struct PParser* p_parser, size_t p_size, PDeclKind p_kind)
{
  PDecl* decl = p_bump_alloc(&p_global_bump_allocator, p_size, P_ALIGNOF(PDecl));
  assert(decl != NULL);
  decl->common.kind = p_kind;
  decl->common.name = NULL;
  decl->common.type = NULL;
  decl->common._llvm_address = NULL;
  decl->common.used = false;
  return decl;
}

#define CREATE_DECL(p_type, p_kind) ((p_type*)create_decl_impl(p_parser, sizeof(p_type), (p_kind)))
#define CREATE_DECL_EXTRA_SIZE(p_type, p_extra_size, p_kind)                                                           \
  ((p_type*)create_decl_impl(p_parser, sizeof(p_type) + (p_extra_size), (p_kind)))

static void
consume_token(struct PParser* p_parser)
{
  p_lex(p_parser->lexer, &p_parser->lookahead);
}

static bool
expect_token(struct PParser* p_parser, PTokenKind p_kind)
{
  if (p_parser->lookahead.kind != p_kind) {
    error("expected '%tk'", p_kind);
    consume_token(p_parser);
    return false;
  }

  consume_token(p_parser);
  return true;
}

static void
unexpected_token(struct PParser* p_parser)
{
  error("unexpected token, got '%tk'", p_parser->lookahead.kind);
}

static bool
skip_until(struct PParser* p_parser, PTokenKind p_kind)
{
  if (expect_token(p_parser, p_kind))
    return true;

  while (!LOOKAHEAD_IS(p_kind) && !LOOKAHEAD_IS(P_TOK_EOF)) {
    consume_token(p_parser);
  }

  return false;
}

static void
skip_until_no_error(struct PParser* p_parser, PTokenKind p_kind)
{
  while (!LOOKAHEAD_IS(p_kind) && !LOOKAHEAD_IS(P_TOK_EOF)) {
    consume_token(p_parser);
  }
}

void
p_parser_init(struct PParser* p_parser)
{
  assert(p_parser != NULL);

  p_parser->loop_depth = 0;

  p_parser->sema.warning_count = 0;
  p_parser->sema.error_count = 0;
  p_parser->sema.current_scope_cache_idx = 0;
  p_parser->sema.current_scope = NULL;
  memset(p_parser->sema.scope_cache, 0, sizeof(p_parser->sema.scope_cache));
}

void
p_parser_destroy(struct PParser* p_parser)
{
  assert(p_parser != NULL);

  assert(p_parser->sema.current_scope == NULL);

  for (size_t i = 0; i < P_MAX_SCOPE_CACHE; ++i) {
    if (p_parser->sema.scope_cache[i] != NULL)
      p_scope_destroy(p_parser->sema.scope_cache[i]);
  }
}

static PAst*
parse_expr(struct PParser* p_parser);
static PAst*
parse_stmt(struct PParser* p_parser);

/*
 * type:
 *     i8
 *     i16
 *     i32
 *     i64
 *     u8
 *     u16
 *     u32
 *     u64
 *     f32
 *     f64
 *     bool
 */
static PType*
parse_type(struct PParser* p_parser)
{
  PType* type;
  switch (p_parser->lookahead.kind) {
    case P_TOK_KEY_i8:
      type = p_type_get_i8();
      break;
    case P_TOK_KEY_i16:
      type = p_type_get_i16();
      break;
    case P_TOK_KEY_i32:
      type = p_type_get_i32();
      break;
    case P_TOK_KEY_i64:
      type = p_type_get_i64();
      break;
    case P_TOK_KEY_u8:
      type = p_type_get_u8();
      break;
    case P_TOK_KEY_u16:
      type = p_type_get_u16();
      break;
    case P_TOK_KEY_u32:
      type = p_type_get_u32();
      break;
    case P_TOK_KEY_u64:
      type = p_type_get_u64();
      break;
    case P_TOK_KEY_f32:
      type = p_type_get_f32();
      break;
    case P_TOK_KEY_f64:
      type = p_type_get_f64();
      break;
    case P_TOK_KEY_bool:
      type = p_type_get_bool();
      break;
    default:
      unexpected_token(p_parser);
      return NULL;
  }

  consume_token(p_parser);
  return type;
}

/*
 * compound_stmt:
 *     "{" stmt_list "}"
 *
 * stmt_list:
 *     stmt
 *     stmt_list stmt
 */
static PAst*
parse_compound_stmt(struct PParser* p_parser)
{
  sema_push_scope(&p_parser->sema);

  expect_token(p_parser, P_TOK_LBRACE);

  PDynamicArray stmts;
  p_dynamic_array_init(&stmts);
  while (!LOOKAHEAD_IS(P_TOK_RBRACE) && !LOOKAHEAD_IS(P_TOK_EOF)) {
    PAst* stmt = parse_stmt(p_parser);
    p_dynamic_array_append(&stmts, stmt);
  }

  expect_token(p_parser, P_TOK_RBRACE);

  PAstCompoundStmt* node =
    CREATE_NODE_EXTRA_SIZE(PAstCompoundStmt, sizeof(PAst*) * (stmts.size - 1), P_AST_NODE_COMPOUND_STMT);
  node->stmt_count = stmts.size;
  memcpy(node->stmts, stmts.buffer, sizeof(PAst*) * stmts.size);
  p_dynamic_array_destroy(&stmts);
  sema_pop_scope(&p_parser->sema);
  return (PAst*)node;
}

/*
 * func_decl:
 *     "fn" identifier "(" param_list ")" type_specifier ";"
 *     "fn" identifier "(" param_list ")" type_specifier compound_stmt
 *
 * param_list:
 *     param
 *     param_list param
 *
 * param:
 *     identifier ":" type
 */
static PDecl*
parse_func_decl(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_fn));
  consume_token(p_parser);

  PIdentifierInfo* identifier_info = NULL;
  if (LOOKAHEAD_IS(P_TOK_IDENTIFIER))
    identifier_info = p_parser->lookahead.data.identifier_info;

  if (!expect_token(p_parser, P_TOK_IDENTIFIER)) {
    skip_until_no_error(p_parser, P_TOK_SEMI);
    return NULL;
  }

  expect_token(p_parser, P_TOK_LPAREN);

  sema_push_scope(&p_parser->sema);
  /* Parse parameters: */
  PDynamicArray params;
  p_dynamic_array_init(&params);
  while (!LOOKAHEAD_IS(P_TOK_RPAREN)) {
    PIdentifierInfo* name = NULL;
    if (LOOKAHEAD_IS(P_TOK_IDENTIFIER))
      name = p_parser->lookahead.data.identifier_info;

    expect_token(p_parser, P_TOK_IDENTIFIER);
    expect_token(p_parser, P_TOK_COLON);
    PType* type = parse_type(p_parser);

    PDeclParam* param = CREATE_DECL(PDeclParam, P_DECL_PARAM);
    P_DECL_GET_NAME(param) = name;
    P_DECL_GET_TYPE(param) = type;
    param->default_expr = NULL;
    p_dynamic_array_append(&params, param);

    sema_check_param_decl(&p_parser->sema, param);

    if (!LOOKAHEAD_IS(P_TOK_COMMA))
      break;
  }

  expect_token(p_parser, P_TOK_RPAREN);

  PType* return_type = NULL;
  if (LOOKAHEAD_IS(P_TOK_COLON)) {
    consume_token(p_parser);
    return_type = parse_type(p_parser);
  }
  
  PDeclFunction* decl = CREATE_DECL_EXTRA_SIZE(PDeclFunction, sizeof(PDeclParam*) * params.size, P_DECL_FUNCTION);
  P_DECL_GET_NAME(decl) = identifier_info;
  decl->param_count = params.size;
  memcpy(decl->params, params.buffer, sizeof(PDeclParam*) * params.size);
  p_dynamic_array_destroy(&params);
  sema_create_func_type(&p_parser->sema, decl, return_type);

  decl->body = parse_compound_stmt(p_parser);

  sema_pop_scope(&p_parser->sema);
  sema_check_func_decl(&p_parser->sema, decl);
  return (PDecl*)decl;
}

/*
 * let_stmt:
 *     "let" identifier type_specifier ";"
 *     "let" identifier type_specifier "=" expr ";"
 *
 * type_specifier:
 *     ""
 *     ":" type
 */
static PAst*
parse_let_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_let));
  consume_token(p_parser);

  PIdentifierInfo* identifier_info = NULL;
  if (LOOKAHEAD_IS(P_TOK_IDENTIFIER))
    identifier_info = p_parser->lookahead.data.identifier_info;

  if (!expect_token(p_parser, P_TOK_IDENTIFIER)) {
    skip_until_no_error(p_parser, P_TOK_SEMI);
    return NULL;
  }

  PType* type = NULL;
  if (LOOKAHEAD_IS(P_TOK_COLON)) {
    consume_token(p_parser);
    type = parse_type(p_parser);
  }

  PAst* init_expr = NULL;
  if (LOOKAHEAD_IS(P_TOK_EQUAL)) {
    consume_token(p_parser);
    init_expr = parse_expr(p_parser);
  }

  skip_until(p_parser, P_TOK_SEMI);

  if (type == NULL) {
    if (init_expr != NULL)
      type = P_AST_GET_TYPE(init_expr);
    else
      type = p_type_get_undef();
  }

  if (init_expr != NULL && type != P_AST_GET_TYPE(init_expr)) {
    error("initialization expression and variable do not have the same type");
  }

  PSymbol* symbol = sema_local_lookup(&p_parser->sema, identifier_info);
  if (symbol != NULL) {
    error("redeclaration of '%s'", identifier_info);
  } else {
    symbol = p_scope_add_symbol(p_parser->sema.current_scope, identifier_info);
    PDeclVar* decl = CREATE_DECL(PDeclVar, P_DECL_VAR);
    symbol->decl = (PDecl*)decl;
    P_DECL_GET_NAME(decl) = identifier_info;
    P_DECL_GET_TYPE(decl) = type;
    decl->init_expr = init_expr;
  }

  PAstLetStmt* node = CREATE_NODE(PAstLetStmt, P_AST_NODE_LET_STMT);
  node->var_decl = symbol->decl;
  return (PAst*)node;
}

/*
 * break_stmt:
 *     "break" ";"
 */
static PAst*
parse_break_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_break));
  consume_token(p_parser);

  expect_token(p_parser, P_TOK_SEMI);

  if (p_parser->loop_depth == 0) {
    error("break must only be used inside a loop");
  }

  PAstBreakStmt* node = CREATE_NODE(PAstBreakStmt, P_AST_NODE_BREAK_STMT);
  sema_check_break_stmt(&p_parser->sema, node);
  return (PAst*)node;
}

/*
 * continue_stmt:
 *     "continue" ";"
 */
static PAst*
parse_continue_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_continue));
  consume_token(p_parser);

  expect_token(p_parser, P_TOK_SEMI);

  if (p_parser->loop_depth == 0) {
    error("continue must only be used inside a loop");
  }

  PAstContinueStmt* node = CREATE_NODE(PAstContinueStmt, P_AST_NODE_CONTINUE_STMT);
  sema_check_continue_stmt(&p_parser->sema, node);
  return (PAst*)node;
}

/*
 * return_stmt:
 *     "return" ";"
 *     "return" expr ";"
 */
static PAst*
parse_return_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_return));
  consume_token(p_parser);

  PAst* ret_expr = NULL;
  if (!LOOKAHEAD_IS(P_TOK_SEMI)) {
    ret_expr = parse_expr(p_parser);
  }

  PAstReturnStmt* node = CREATE_NODE(PAstReturnStmt, P_AST_NODE_RETURN_STMT);
  node->ret_expr = ret_expr;
  sema_check_return_stmt(&p_parser->sema, node);

  expect_token(p_parser, P_TOK_SEMI);
  return (PAst*)node;
}

/*
 * if_stmt:
 *     "if" "(" expr ")" compound_stmt
 *     "if" "(" expr ")" compound_stmt "else" compound_stmt
 */
static PAst*
parse_if_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_if));
  consume_token(p_parser);

  expect_token(p_parser, P_TOK_LPAREN);
  PAst* cond_expr = parse_expr(p_parser);
  expect_token(p_parser, P_TOK_RPAREN);

  PAst* then_stmt = parse_compound_stmt(p_parser);

  PAst* else_stmt = NULL;
  if (LOOKAHEAD_IS(P_TOK_KEY_else)) {
    consume_token(p_parser);
    else_stmt = parse_compound_stmt(p_parser);
  }

  PAstIfStmt* node = CREATE_NODE(PAstIfStmt, P_AST_NODE_IF_STMT);
  node->cond_expr = cond_expr;
  node->then_stmt = then_stmt;
  node->else_stmt = else_stmt;
  sema_check_if_stmt(&p_parser->sema, node);
  return (PAst*)node;
}

/*
 * while_stmt:
 *     "while" "(" expr ")" compound_stmt
 */
static PAst*
parse_while_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_while));
  consume_token(p_parser);

  expect_token(p_parser, P_TOK_LPAREN);
  PAst* cond_expr = parse_expr(p_parser);
  expect_token(p_parser, P_TOK_RPAREN);

  ++p_parser->loop_depth;
  PAst* body_stmt = parse_compound_stmt(p_parser);
  --p_parser->loop_depth;

  PAstWhileStmt* node = CREATE_NODE(PAstWhileStmt, P_AST_NODE_WHILE_STMT);
  node->cond_expr = cond_expr;
  node->body_stmt = body_stmt;
  sema_check_while_stmt(&p_parser->sema, node);
  return (PAst*)node;
}

/*
 * expr_stmt:
 *     expr ";"
 */
static PAst*
parse_expr_stmt(struct PParser* p_parser)
{
  PAst* expr = parse_expr(p_parser);
  expect_token(p_parser, P_TOK_SEMI);
  return expr;
}

/*
 * stmt:
 *     compound_stmt
 *     let_stmt
 *     break_stmt
 *     continue_stmt
 *     return_stmt
 *     if_stmt
 *     while_stmt
 *     expr_stmt
 */
static PAst*
parse_stmt(struct PParser* p_parser)
{
  switch (p_parser->lookahead.kind) {
    case P_TOK_LBRACE:
      return parse_compound_stmt(p_parser);
    case P_TOK_KEY_let:
      return parse_let_stmt(p_parser);
    case P_TOK_KEY_break:
      return parse_break_stmt(p_parser);
    case P_TOK_KEY_continue:
      return parse_continue_stmt(p_parser);
    case P_TOK_KEY_return:
      return parse_return_stmt(p_parser);
    case P_TOK_KEY_if:
      return parse_if_stmt(p_parser);
    case P_TOK_KEY_while:
      return parse_while_stmt(p_parser);
    default:
      return parse_expr_stmt(p_parser);
  }
}

/*
 * bool_literal:
 *     "true"
 *     "false"
 */
static PAst*
parse_bool_literal(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_true) || LOOKAHEAD_IS(P_TOK_KEY_false));

  const bool value = LOOKAHEAD_IS(P_TOK_KEY_true);
  consume_token(p_parser);

  PAstBoolLiteral* node = CREATE_NODE(PAstBoolLiteral, P_AST_NODE_BOOL_LITERAL);
  P_AST_GET_TYPE(node) = p_type_get_bool();
  node->value = value;
  return (PAst*)node;
}

/*
 * int_literal
 */
static PAst*
parse_int_literal(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_INT_LITERAL));

  unsigned long long value = 0;
  for (const char* it = p_parser->lookahead.data.literal_begin; it != p_parser->lookahead.data.literal_end; ++it) {
    value *= 10;
    value += *it - '0';
  }

  consume_token(p_parser);

  PAstIntLiteral* node = CREATE_NODE(PAstIntLiteral, P_AST_NODE_INT_LITERAL);
  P_AST_GET_TYPE(node) = p_type_get_i32();
  node->value = value;
  sema_check_int_literal(&p_parser->sema, node);
  return (PAst*)node;
}

/*
 * float_literal
 */
static PAst*
parse_float_literal(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_FLOAT_LITERAL));
  consume_token(p_parser);

  PAstFloatLiteral* node = CREATE_NODE(PAstFloatLiteral, P_AST_NODE_FLOAT_LITERAL);
  P_AST_GET_TYPE(node) = p_type_get_f32();
  // TODO: parse value of float literal
  return (PAst*)node;
}

/*
 * paren_expr:
 *     "(" expr ")"
 */
static PAst*
parse_paren_expr(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_LPAREN));
  consume_token(p_parser);

  PAstParenExpr* node = CREATE_NODE(PAstParenExpr, P_AST_NODE_PAREN_EXPR);
  node->sub_expr = parse_expr(p_parser);
  P_AST_GET_TYPE(node) = P_AST_GET_TYPE(node->sub_expr);

  expect_token(p_parser, P_TOK_RPAREN);
  return (PAst*)node;
}

/*
 * primary_expr:
 *     identifier "(" arg_list ")"
 *     identifier
 *
 * arg_list:
 *     expr
 *     arg_list "," expr
 */
static PAst*
parse_var_ref_or_call_expr(struct PParser* p_parser)
{
  assert(p_parser->lookahead.kind == P_TOK_IDENTIFIER);

  PIdentifierInfo* name = p_parser->lookahead.data.identifier_info;
  consume_token(p_parser);

  PSymbol* symbol = sema_lookup(&p_parser->sema, name);
  PDecl* decl = NULL;
  if (symbol != NULL)
    decl = symbol->decl;

  if (LOOKAHEAD_IS(P_TOK_LPAREN)) {
    /* Is a function call expr. */
    consume_token(p_parser); /* consume '(' */

    if (symbol == NULL) {
      error("use of undeclared function '%i'");
    } else if (P_DECL_GET_KIND(decl) != P_DECL_FUNCTION) {
      error("'%i' is not a function");
    }

    if (decl != NULL)
      decl->common.used = true;

    /* Parse arguments: */
    PDynamicArray args;
    p_dynamic_array_init(&args);

    int param_count = decl != NULL ? P_DECL_GET_TYPE(decl)->func_ty.arg_count : 0;
    PType** param_types = decl != NULL ? P_DECL_GET_TYPE(decl)->func_ty.args : NULL;

    int i = 0;
    while (!LOOKAHEAD_IS(P_TOK_RPAREN)) {
      PAst* arg = parse_expr(p_parser);
      p_dynamic_array_append(&args, arg);

      if (i < param_count && P_AST_GET_TYPE(arg) != param_types[i]) {
        error("%d-th argument of '%i' expected to be of type '%ty' but is of "
              "type '%ty'",
              i,
              name,
              param_types[i],
              P_AST_GET_TYPE(arg));
      }

      ++i;

      if (!LOOKAHEAD_IS(P_TOK_COMMA))
        break;

      consume_token(p_parser); /* consume ',' */
    }

    if (param_count != args.size) {
      error("function '%i' expects %d arguments but %d were provided", name, param_count, (int)args.size);
    }

    expect_token(p_parser, P_TOK_RPAREN);

    PAstCallExpr* node = CREATE_NODE_EXTRA_SIZE(PAstCallExpr, sizeof(PAst*) * (args.size - 1), P_AST_NODE_CALL_EXPR);
    P_AST_GET_TYPE(node) = P_DECL_GET_TYPE(decl)->func_ty.ret;
    node->func = decl;
    node->arg_count = args.size;
    memcpy(node->args, args.buffer, sizeof(PAst*) * args.size);
    p_dynamic_array_destroy(&args);
    return (PAst*)node;
  } else {
    /* Is a var ref expr. */

    if (symbol == NULL) {
      error("use of undeclared variable '%i'", name);
    } else if (P_DECL_GET_KIND(symbol->decl) != P_DECL_VAR && P_DECL_GET_KIND(symbol->decl) != P_DECL_PARAM) {
      error("'%i' is not a variable", name);
    }
    
    PAstDeclRefExpr* node = CREATE_NODE(PAstDeclRefExpr, P_AST_NODE_DECL_REF_EXPR);
    node->decl = decl;

    if (decl == NULL) {
      P_AST_GET_TYPE(node) = NULL;
    } else {
      P_AST_GET_TYPE(node) = P_DECL_GET_TYPE(decl);
      decl->common.used = true;
    }

    return (PAst*)node;
  }
}

/*
 * primary_expr:
 *     bool_literal
 *     int_literal
 *     float_literal
 *     paren_expr
 *     identifier "(" arg_list ")"
 *     identifier
 */
static PAst*
parse_primary_expr(struct PParser* p_parser)
{
  switch (p_parser->lookahead.kind) {
    case P_TOK_INT_LITERAL:
      return parse_int_literal(p_parser);
    case P_TOK_FLOAT_LITERAL:
      return parse_float_literal(p_parser);
    case P_TOK_KEY_true:
    case P_TOK_KEY_false:
      return parse_bool_literal(p_parser);
    case P_TOK_LPAREN:
      return parse_paren_expr(p_parser);
    case P_TOK_IDENTIFIER:
      return parse_var_ref_or_call_expr(p_parser);
    default:
      unexpected_token(p_parser);
      return NULL;
  }
}

static int
get_binop_precedence(PTokenKind p_op)
{
  switch (p_op) {
#define BINOP(p_kind, p_tok, p_precedence)                                                                             \
  case p_tok:                                                                                                          \
    return p_precedence;
#include "operator_kinds.def"
    default:
      return -1;
  }
}

static PAstBinaryOp
get_binop_from_tok_kind(PTokenKind p_op)
{
  switch (p_op) {
#define BINOP(p_kind, p_tok, p_precedence)                                                                             \
  case p_tok:                                                                                                          \
    return p_kind;
#include "operator_kinds.def"
    default:
      HEDLEY_UNREACHABLE_RETURN(P_BINARY_ADD);
  }
}

static PAst*
parse_binary_expr(struct PParser* p_parser, PAst* p_lhs, int p_expr_prec)
{
  while (true) {
    int tok_prec = get_binop_precedence(p_parser->lookahead.kind);

    /* If this is a binop that binds at least as tightly as the current
     * binary_op, consume it, otherwise we are done. */
    if (tok_prec < p_expr_prec)
      return p_lhs;

    /* Okay, we know this is a binary_op. */
    const PAstBinaryOp binary_op = get_binop_from_tok_kind(p_parser->lookahead.kind);
    consume_token(p_parser);

    /* Parse the primary expression after the binary operator. */
    PAst* rhs = parse_primary_expr(p_parser);
    if (rhs == NULL)
      return NULL;

    /* If BinOp binds less tightly with RHS than the operator after RHS, let
     * the pending operator take RHS as its LHS. */
    int new_prec = get_binop_precedence(p_parser->lookahead.kind);
    if (tok_prec < new_prec) {
      rhs = parse_binary_expr(p_parser, rhs, tok_prec + 1);
      if (rhs == NULL)
        return NULL;
    }

    /* Merge lhs / rhs. */
    PAstBinaryExpr* new_lhs = CREATE_NODE(PAstBinaryExpr, P_AST_NODE_BINARY_EXPR);
    new_lhs->lhs = p_lhs;
    new_lhs->rhs = rhs;
    new_lhs->opcode = binary_op;
    sema_check_binary_expr(&p_parser->sema, new_lhs);
    p_lhs = (PAst*)new_lhs;
  }
}

static PAst*
parse_expr(struct PParser* p_parser)
{
  PAst* lhs = parse_primary_expr(p_parser);
  return parse_binary_expr(p_parser, lhs, 0);
}

/*
 * translation_unit:
 *     function_decls
 *
 * function_decls:
 *     function_decl
 *     function_decls function_decl
 */
static PAst*
parse_translation_unit(struct PParser* p_parser)
{
  sema_push_scope(&p_parser->sema);

  PDynamicArray decls;
  p_dynamic_array_init(&decls);
  while (!LOOKAHEAD_IS(P_TOK_EOF)) {
    PDecl* func = parse_func_decl(p_parser);
    p_dynamic_array_append(&decls, func);
  }

  PAstTranslationUnit* node =
    CREATE_NODE_EXTRA_SIZE(PAstTranslationUnit, sizeof(PDecl*) * (decls.size - 1), P_AST_NODE_TRANSLATION_UNIT);
  node->decl_count = decls.size;
  memcpy(node->decls, decls.buffer, sizeof(PDecl*) * decls.size);

  p_dynamic_array_destroy(&decls);
  sema_pop_scope(&p_parser->sema);
  return (PAst*)node;
}

PAst*
p_parse(struct PParser* p_parser)
{
  assert(p_parser != NULL);

  consume_token(p_parser);
  PAst* ast = parse_translation_unit(p_parser);
  return ast;
}
