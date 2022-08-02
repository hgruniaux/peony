#include "parser.h"

#include "options.h"
#include "scope.h"

#include "utils/bump_allocator.h"
#include "utils/diag.h"
#include "utils/dynamic_array.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define LOOKAHEAD_IS(p_kind) ((p_parser)->lookahead.kind == (p_kind))
#define LOOKAHEAD_BEGIN_LOC ((p_parser)->lookahead.source_location)
#define LOOKAHEAD_END_LOC ((p_parser)->lookahead.source_location + (p_parser)->lookahead.token_length)
#define SET_NODE_LOC_RANGE(p_node, p_loc_beg, p_loc_end)                                                               \
  (P_AST_GET_SOURCE_RANGE(p_node).begin = (p_loc_beg));                                                                \
  (P_AST_GET_SOURCE_RANGE(p_node).end = (p_loc_end))

static const char*
trim_whitespace_left(const char* p_it)
{
  while (*p_it == ' ' || *p_it == '\t')
    ++p_it;

  return p_it;
}

static const char*
trim_whitespace_right(const char* p_it)
{
  while (p_it[-1] == ' ' || p_it[-1] == '\t')
    --p_it;

  return p_it;
}

static void
consume_token(struct PParser* p_parser)
{
  p_lex(p_parser->lexer, &p_parser->lookahead);

  if (LOOKAHEAD_IS(P_TOK_COMMENT) && g_options.opt_verify_mode) {
    const char* it = p_parser->lookahead.data.literal_begin;
    while (*it == ' ' || *it == '\t')
      ++it;

    const char* expect_error_marker = "EXPECT-ERROR:";
    const char* expect_warning_marker = "EXPECT-WARNING:";

    if (memcmp(it, expect_error_marker, strlen(expect_error_marker)) == 0) {
      it += strlen(expect_error_marker);
      it = trim_whitespace_left(it);
      const char* end = p_parser->lookahead.data.literal_end;
      end = trim_whitespace_right(end);
      verify_expect_error(it, end);
    } else if (memcmp(it, expect_warning_marker, strlen(expect_warning_marker)) == 0) {
      it += strlen(expect_warning_marker);
      it = trim_whitespace_left(it);
      const char* end = p_parser->lookahead.data.literal_end;
      end = trim_whitespace_right(end);
      verify_expect_warning(it, end);
    }

    consume_token(p_parser);
  }
}

static void
print_expect_tok_diag(struct PParser* p_parser, PTokenKind p_kind)
{
  PDiag* d = diag_at(P_DK_err_expected_tok, LOOKAHEAD_BEGIN_LOC);
  diag_add_arg_tok_kind(d, p_kind);
  diag_add_arg_tok_kind(d, p_parser->lookahead.kind);
  diag_flush(d);
}

static bool
expect_token(struct PParser* p_parser, PTokenKind p_kind)
{
  if (p_parser->lookahead.kind != p_kind) {
    print_expect_tok_diag(p_parser, p_kind);
    consume_token(p_parser);
    return false;
  }

  consume_token(p_parser);
  return true;
}

static void
unexpected_token(struct PParser* p_parser)
{
  PDiag* d = diag_at(P_DK_err_unexpected_tok, LOOKAHEAD_BEGIN_LOC);
  diag_add_arg_tok_kind(d, p_parser->lookahead.kind);
  diag_flush(d);
  consume_token(p_parser);
}

static void
skip_until_no_error(struct PParser* p_parser, PTokenKind p_kind)
{
  while (!LOOKAHEAD_IS(p_kind) && !LOOKAHEAD_IS(P_TOK_EOF))
    consume_token(p_parser);
}

static bool
skip_until(struct PParser* p_parser, PTokenKind p_kind)
{
  if (expect_token(p_parser, p_kind))
    return true;

  skip_until_no_error(p_parser, p_kind);

  return false;
}

static bool
skip_until_no_consume(struct PParser* p_parser, PTokenKind p_kind)
{
  if (LOOKAHEAD_IS(p_kind))
    return true;

  print_expect_tok_diag(p_parser, p_kind);

  while (!LOOKAHEAD_IS(p_kind) && !LOOKAHEAD_IS(P_TOK_EOF))
    consume_token(p_parser);

  return false;
}

void
p_parser_init(struct PParser* p_parser)
{
  assert(p_parser != NULL);

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
 *     char
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
 *     "*" type
 *     "(" type ")"
 */
static PType*
parse_type(struct PParser* p_parser)
{
  if (LOOKAHEAD_IS(P_TOK_STAR)) {
    consume_token(p_parser);
    return p_type_get_pointer(parse_type(p_parser));
  } else if (LOOKAHEAD_IS(P_TOK_LPAREN)) {
    consume_token(p_parser);
    PType* sub_type = parse_type(p_parser);
    expect_token(p_parser, P_TOK_RPAREN);
    return p_type_get_paren(sub_type);
  }

  PType* type;
  switch (p_parser->lookahead.kind) {
    case P_TOK_KEY_char:
      type = p_type_get_char();
      break;
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
  sema_push_scope(&p_parser->sema, P_SF_NONE);

  PSourceLocation lbrace_loc = LOOKAHEAD_BEGIN_LOC;
  expect_token(p_parser, P_TOK_LBRACE);

  PDynamicArray stmts;
  p_dynamic_array_init(&stmts);
  while (!LOOKAHEAD_IS(P_TOK_RBRACE) && !LOOKAHEAD_IS(P_TOK_EOF)) {
    PAst* stmt = parse_stmt(p_parser);
    p_dynamic_array_append(&stmts, stmt);
  }

  PSourceLocation rbrace_loc = LOOKAHEAD_BEGIN_LOC;
  expect_token(p_parser, P_TOK_RBRACE);

  PAstCompoundStmt* node =
    CREATE_NODE_EXTRA_SIZE(PAstCompoundStmt, sizeof(PAst*) * (stmts.size - 1), P_AST_NODE_COMPOUND_STMT);
  node->stmt_count = stmts.size;
  memcpy(node->stmts, stmts.buffer, sizeof(PAst*) * stmts.size);
  p_dynamic_array_destroy(&stmts);
  SET_NODE_LOC_RANGE(node, lbrace_loc, rbrace_loc + 1);
  sema_pop_scope(&p_parser->sema);
  return (PAst*)node;
}

/*
 * param_decl:
 *     identifier "=" expr
 *     identifier type_specifier ("=" expr)?
 *
 * var_decl:
 *     identifier "=" expr
 *     identifier type_specifier ("=" expr)?
 *
 * type_specifier:
 *     ":" type
 */
static PDecl*
parse_param_or_var_decl(struct PParser* p_parser, bool p_is_param)
{
  PIdentifierInfo* name = NULL;
  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  if (!LOOKAHEAD_IS(P_TOK_IDENTIFIER)) {
    PDiag* d = diag_at(p_is_param ? P_DK_err_expected_param_decl : P_DK_err_expected_var_decl, LOOKAHEAD_BEGIN_LOC);
    diag_flush(d);
    skip_until_no_consume(p_parser, P_TOK_RPAREN);
    return NULL;
  }

  name = p_parser->lookahead.data.identifier_info;

  expect_token(p_parser, P_TOK_IDENTIFIER);

  PType* type = NULL;
  if (LOOKAHEAD_IS(P_TOK_COLON)) {
    consume_token(p_parser); // consume ':'
    type = parse_type(p_parser);
  }

  PAst* default_expr = NULL;
  if (LOOKAHEAD_IS(P_TOK_EQUAL)) {
    consume_token(p_parser); // consume '='
    default_expr = parse_expr(p_parser);
  }

  if (p_is_param)
    return (PDecl*)sema_act_on_param_decl(&p_parser->sema, name_range, name, type, default_expr);
  else
    return (PDecl*)sema_act_on_var_decl(&p_parser->sema, name_range, name, type, default_expr);
}

/*
 * param_decl_list:
 *     param_decl
 *     param_decl_list "," param_decl
 *
 * var_decl_list:
 *     var_decl
 *     var_decl_list "," var_decl
 */
static void
parse_param_or_var_list(struct PParser* p_parser, PDynamicArray* p_param_list, bool p_is_param)
{
  while (true) {
    PDecl* decl = parse_param_or_var_decl(p_parser, p_is_param);
    if (decl != NULL)
      p_dynamic_array_append(p_param_list, decl);

    if (p_is_param ? LOOKAHEAD_IS(P_TOK_RPAREN) : LOOKAHEAD_IS(P_TOK_SEMI))
      break;

    expect_token(p_parser, P_TOK_COMMA);
  }
}

/*
 * func_decl:
 *     "fn" identifier "(" param_decl_list? ")" func_type_specifier? ";"
 *     "fn" identifier "(" param_decl_list? ")" func_type_specifier? compound_stmt
 *
 * func_type_specifier:
 *     "->" type
 */
static PDecl*
parse_func_decl(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_fn));
  consume_token(p_parser);

  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = NULL;
  if (LOOKAHEAD_IS(P_TOK_IDENTIFIER)) {
    name = p_parser->lookahead.data.identifier_info;
    consume_token(p_parser); // consume identifier
  } else {
    // Act as if there was an identifier but emit a diagnostic.
    print_expect_tok_diag(p_parser, P_TOK_IDENTIFIER);
  }

  PDynamicArray params;
  p_dynamic_array_init(&params);

  // Parse parameters
  sema_push_scope(&p_parser->sema, P_SF_NONE);
  expect_token(p_parser, P_TOK_LPAREN);
  if (!LOOKAHEAD_IS(P_TOK_RPAREN))
    parse_param_or_var_list(p_parser, &params, /* is_param */ true);
  expect_token(p_parser, P_TOK_RPAREN);
  sema_pop_scope(&p_parser->sema);

  // Parse return type specified (optional)
  PType* return_type = NULL;
  if (LOOKAHEAD_IS(P_TOK_ARROW)) {
    consume_token(p_parser); // consume '->'
    return_type = parse_type(p_parser);
  }

  PDeclFunction* decl =
    sema_act_on_func_decl(&p_parser->sema, name_range, name, return_type, (PDeclParam**)params.buffer, params.size);
  p_dynamic_array_destroy(&params);

  // Parse body
  // TODO: lazy body parsing
  sema_begin_func_decl_body_parsing(&p_parser->sema, decl);
  PAst* body = parse_compound_stmt(p_parser);
  sema_end_func_decl_body_parsing(&p_parser->sema, decl);

  decl->body = body;
  return (PDecl*)decl;
}

/*
 * let_stmt:
 *     "let" var_decl_list ";"
 */
static PAst*
parse_let_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_let));

  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser);

  PDynamicArray var_decls;
  p_dynamic_array_init(&var_decls);
  parse_param_or_var_list(p_parser, &var_decls, /* is_param */ false);

  expect_token(p_parser, P_TOK_SEMI);

  PAstLetStmt* node = sema_act_on_let_stmt(&p_parser->sema, (PDeclVar**)var_decls.buffer, var_decls.size);
  p_dynamic_array_destroy(&var_decls);
  if (node == NULL)
    return NULL;

  // FIXME: incorrect end source location
  SET_NODE_LOC_RANGE(node, loc_begin, LOOKAHEAD_BEGIN_LOC);
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

  PSourceRange break_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  consume_token(p_parser);

  PSourceLocation loc_end = LOOKAHEAD_END_LOC;
  expect_token(p_parser, P_TOK_SEMI);

  PAstBreakStmt* node = sema_act_on_break_stmt(&p_parser->sema, break_range);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, break_range.begin, loc_end);
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

  PSourceRange continue_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  consume_token(p_parser);

  PSourceLocation loc_end = LOOKAHEAD_END_LOC;
  expect_token(p_parser, P_TOK_SEMI);

  PAstContinueStmt* node = sema_act_on_continue_stmt(&p_parser->sema, continue_range);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, continue_range.begin, loc_end);
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

  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser);

  PAst* ret_expr = NULL;
  if (!LOOKAHEAD_IS(P_TOK_SEMI))
    ret_expr = parse_expr(p_parser);

  PSourceLocation loc_end = LOOKAHEAD_END_LOC;
  expect_token(p_parser, P_TOK_SEMI);

  PAstReturnStmt* node = sema_act_on_return_stmt(&p_parser->sema, loc_end, ret_expr);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, loc_begin, loc_end);
  return (PAst*)node;
}

/*
 * if_stmt:
 *     "if" expr compound_stmt
 *     "if" expr compound_stmt "else" (compound_stmt | if_stmt)
 */
static PAst*
parse_if_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_if));

  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser);

  PAst* cond_expr = parse_expr(p_parser);
  PAst* then_stmt = parse_compound_stmt(p_parser);

  PSourceLocation loc_end;
  PAst* else_stmt = NULL;
  if (LOOKAHEAD_IS(P_TOK_KEY_else)) {
    consume_token(p_parser);

    if (LOOKAHEAD_IS(P_TOK_KEY_if)) {
      else_stmt = parse_if_stmt(p_parser);
    } else {
      else_stmt = parse_compound_stmt(p_parser);
    }

    if (else_stmt != NULL)
      loc_end = P_AST_GET_SOURCE_RANGE(else_stmt).end;
  } else if (then_stmt != NULL) {
    loc_end = P_AST_GET_SOURCE_RANGE(then_stmt).end;
  }

  PAstIfStmt* node = sema_act_on_if_stmt(&p_parser->sema, cond_expr, then_stmt, else_stmt);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, loc_begin, loc_end);
  return (PAst*)node;
}

/*
 * loop_stmt:
 *     "loop" compound_stmt
 */
static PAst*
parse_loop_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_loop));

  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser);

  sema_push_scope(&p_parser->sema, P_SF_BREAK | P_SF_CONTINUE);

  PAstLoopStmt* node = sema_act_on_loop_stmt(&p_parser->sema, p_parser->sema.current_scope);
  if (node == NULL) {
    parse_compound_stmt(p_parser);
    sema_pop_scope(&p_parser->sema);
    return NULL;
  }

  node->body_stmt = parse_compound_stmt(p_parser);
  if (node->body_stmt == NULL) {
    sema_pop_scope(&p_parser->sema);
    return NULL;
  }

  SET_NODE_LOC_RANGE(node, loc_begin, P_AST_GET_SOURCE_RANGE(node->body_stmt).end);
  sema_pop_scope(&p_parser->sema);
  return (PAst*)node;
}

/*
 * while_stmt:
 *     "while" expr compound_stmt
 */
static PAst*
parse_while_stmt(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_while));

  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser);

  sema_push_scope(&p_parser->sema, P_SF_BREAK | P_SF_CONTINUE);

  PAst* cond_expr = parse_expr(p_parser);

  PAstWhileStmt* node = sema_act_on_while_stmt(&p_parser->sema, cond_expr, p_parser->sema.current_scope);
  if (node == NULL) {
    parse_compound_stmt(p_parser);
    sema_pop_scope(&p_parser->sema);
    return NULL;
  }

  node->body_stmt = parse_compound_stmt(p_parser);
  if (node->body_stmt == NULL) {
    sema_pop_scope(&p_parser->sema);
    return NULL;
  }

  SET_NODE_LOC_RANGE(node, loc_begin, P_AST_GET_SOURCE_RANGE(node->body_stmt).end);
  sema_pop_scope(&p_parser->sema);
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
 *     loop_stmt
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
    case P_TOK_KEY_loop:
      return parse_loop_stmt(p_parser);
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

  PSourceRange range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };

  const bool value = LOOKAHEAD_IS(P_TOK_KEY_true);
  consume_token(p_parser);

  PAstBoolLiteral* node = sema_act_on_bool_literal(&p_parser->sema, range, value);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, range.begin, range.end);
  return (PAst*)node;
}

/*
 * int_literal
 */
static PAst*
parse_int_literal(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_INT_LITERAL));

  PSourceRange range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };

  uintmax_t value = 0;
  for (const char* it = p_parser->lookahead.data.literal_begin; it != p_parser->lookahead.data.literal_end; ++it) {
    value *= 10;
    value += *it - '0';
  }

  consume_token(p_parser);

  PAstIntLiteral* node = sema_act_on_int_literal(&p_parser->sema, range, p_type_get_i32(), value);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, range.begin, range.end);
  return (PAst*)node;
}

/*
 * float_literal
 */
static PAst*
parse_float_literal(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_FLOAT_LITERAL));

  PSourceRange range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  consume_token(p_parser);

  PAstFloatLiteral* node = sema_act_on_float_literal(&p_parser->sema, range, p_type_get_f32(), 0.0);
  if (node == NULL)
    return NULL;

  // TODO: parse value of float literal
  SET_NODE_LOC_RANGE(node, range.begin, range.end);
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

  PSourceLocation lparen_loc = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser);

  PAst* sub_expr = parse_expr(p_parser);

  PSourceLocation rparen_loc = LOOKAHEAD_BEGIN_LOC;
  expect_token(p_parser, P_TOK_RPAREN);

  PAstParenExpr* node = sema_act_on_paren_expr(&p_parser->sema, lparen_loc, rparen_loc, sub_expr);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, lparen_loc, rparen_loc + 1);
  return (PAst*)node;
}

/*
 * decl_ref_expr:
 *     identifier
 */
static PAst*
parse_decl_ref_expr(struct PParser* p_parser)
{
  assert(p_parser->lookahead.kind == P_TOK_IDENTIFIER);

  PSourceRange range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = p_parser->lookahead.data.identifier_info;
  consume_token(p_parser);

  PAstDeclRefExpr* node = sema_act_on_decl_ref_expr(&p_parser->sema, range, name);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, range.begin, range.end);
  return (PAst*)node;
}

/*
 * primary_expr:
 *     bool_literal
 *     int_literal
 *     float_literal
 *     paren_expr
 *     decl_ref_expr
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
      return parse_decl_ref_expr(p_parser);
    default:
      unexpected_token(p_parser);
      return NULL;
  }
}

/*
 * call_expr:
 *     primary_expr
 *     primary_expr "(" arg_list ")"
 *
 * arg_list:
 *     expr
 *     arg_list "," expr
 */
static PAst*
parse_call_expr(struct PParser* p_parser)
{
  PAst* callee = parse_primary_expr(p_parser);
  if (!LOOKAHEAD_IS(P_TOK_LPAREN))
    return callee;

  PSourceLocation lparen_loc = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser); /* consume '(' */

  /* Parse arguments: */
  PDynamicArray args;
  p_dynamic_array_init(&args);
  while (!LOOKAHEAD_IS(P_TOK_RPAREN)) {
    PAst* arg = parse_expr(p_parser);
    p_dynamic_array_append(&args, arg);

    if (!LOOKAHEAD_IS(P_TOK_COMMA))
      break;

    consume_token(p_parser); /* consume ',' */
  }

  PSourceLocation rparen_loc = LOOKAHEAD_BEGIN_LOC;
  expect_token(p_parser, P_TOK_RPAREN);

  PAstCallExpr* node =
    sema_act_on_call_expr(&p_parser->sema, lparen_loc, rparen_loc, callee, (PAst**)args.buffer, args.size);
  p_dynamic_array_destroy(&args);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, P_AST_GET_SOURCE_RANGE(callee).begin, rparen_loc + 1);
  return (PAst*)node;
}

/*
 * unary_expr:
 *     call_expr
 *     "-" unary_expr
 *     "!" unary_expr
 *     "&" unary_expr
 *     "*" deref_expr
 */
static PAst*
parse_unary_expr(struct PParser* p_parser)
{
  PAstUnaryOp opcode;
  switch (p_parser->lookahead.kind) {
    case P_TOK_EXCLAIM:
      opcode = P_UNARY_NOT;
      break;
    case P_TOK_MINUS:
      opcode = P_UNARY_NEG;
      break;
    case P_TOK_AMP:
      opcode = P_UNARY_ADDRESS_OF;
      break;
    case P_TOK_STAR:
      opcode = P_UNARY_DEREF;
      break;
    default:
      return parse_call_expr(p_parser);
  }

  PSourceLocation op_loc = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser); // consume opcode

  PAst* sub_expr = parse_unary_expr(p_parser);
  PAstUnaryExpr* node = sema_act_on_unary_expr(&p_parser->sema, opcode, op_loc, sub_expr);
  if (node == NULL)
    return NULL;

  SET_NODE_LOC_RANGE(node, op_loc, P_AST_GET_SOURCE_RANGE(sub_expr).end);
  return (PAst*)node;
}

/*
 * cast_expr:
 *     unary_expr
 *     unary_expr "as" type
 */
static PAst*
parse_cast_expr(struct PParser* p_parser)
{
  PAst* sub_expr = parse_unary_expr(p_parser);

  if (LOOKAHEAD_IS(P_TOK_KEY_as)) {
    PSourceLocation as_loc = LOOKAHEAD_BEGIN_LOC;
    consume_token(p_parser);
    PType* to_type = parse_type(p_parser);
    PAstCastExpr* node = sema_act_on_cast_expr(&p_parser->sema, as_loc, to_type, sub_expr);
    if (node == NULL)
      return NULL;

    // FIXME: better end source location for cast expr instead of LOOKAHEAD_END_LOC
    //        probably use type->source_range.end when types have localizations.
    SET_NODE_LOC_RANGE(node, P_AST_GET_SOURCE_RANGE(sub_expr).begin, LOOKAHEAD_END_LOC);
    return (PAst*)node;
  }

  return sub_expr;
}

static int
get_binop_precedence(PTokenKind p_op)
{
  switch (p_op) {
#define BINARY_OPERATOR(p_kind, p_tok, p_precedence)                                                                   \
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
#define BINARY_OPERATOR(p_kind, p_tok, p_precedence)                                                                   \
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
    const PAstBinaryOp binary_opcode = get_binop_from_tok_kind(p_parser->lookahead.kind);
    const PSourceLocation op_loc = LOOKAHEAD_BEGIN_LOC;
    consume_token(p_parser);

    /* Parse the primary expression after the binary operator. */
    PAst* rhs = parse_cast_expr(p_parser);
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
    PAstBinaryExpr* new_lhs = sema_act_on_binary_expr(&p_parser->sema, binary_opcode, op_loc, p_lhs, rhs);
    if (new_lhs == NULL)
      return NULL;

    SET_NODE_LOC_RANGE(new_lhs, P_AST_GET_SOURCE_RANGE(p_lhs).begin, P_AST_GET_SOURCE_RANGE(rhs).end);
    p_lhs = (PAst*)new_lhs;
  }
}

static PAst*
parse_expr(struct PParser* p_parser)
{
  PAst* lhs = parse_cast_expr(p_parser);
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
  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  sema_push_scope(&p_parser->sema, P_SF_NONE);

  PDynamicArray decls;
  p_dynamic_array_init(&decls);
  while (!LOOKAHEAD_IS(P_TOK_EOF)) {
    PDecl* func = parse_func_decl(p_parser);
    p_dynamic_array_append(&decls, func);
  }

  expect_token(p_parser, P_TOK_EOF);

  PAstTranslationUnit* node =
    CREATE_NODE_EXTRA_SIZE(PAstTranslationUnit, sizeof(PDecl*) * (decls.size - 1), P_AST_NODE_TRANSLATION_UNIT);
  node->decl_count = decls.size;
  memcpy(node->decls, decls.buffer, sizeof(PDecl*) * decls.size);

  p_dynamic_array_destroy(&decls);
  sema_pop_scope(&p_parser->sema);
  SET_NODE_LOC_RANGE(node, loc_begin, LOOKAHEAD_END_LOC);
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
