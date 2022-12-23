#include "parser.hxx"

#include "literal_parser.hxx"
#include "scope.hxx"

#include "utils/diag.hxx"

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <vector>

#define LOOKAHEAD_IS(p_kind) ((p_parser)->lookahead.kind == (p_kind))
#define LOOKAHEAD_BEGIN_LOC ((p_parser)->lookahead.source_location)
#define LOOKAHEAD_END_LOC ((p_parser)->lookahead.source_location + (p_parser)->lookahead.token_length)
#define SET_NODE_LOC_RANGE(p_node, p_loc_beg, p_loc_end)                                                               \
  (P_AST_GET_SOURCE_RANGE(p_node).begin = (p_loc_beg));                                                                \
  (P_AST_GET_SOURCE_RANGE(p_node).end = (p_loc_end))

#define AST_LIST_INIT(p_array) DYN_ARRAY_INIT(PAst*, p_array)
#define AST_LIST_DESTROY(p_array) DYN_ARRAY_DESTROY(PAst*, p_array)
#define AST_LIST_APPEND(p_array, p_item) DYN_ARRAY_APPEND(PAst*, p_array, p_item)
#define AST_LIST_AT(p_array, p_idx) DYN_ARRAY_AT(PAst*, p_array, p_idx)

#define DECL_LIST_INIT(p_array) DYN_ARRAY_INIT(PDecl*, p_array)
#define DECL_LIST_DESTROY(p_array) DYN_ARRAY_DESTROY(PDecl*, p_array)
#define DECL_LIST_APPEND(p_array, p_item) DYN_ARRAY_APPEND(PDecl*, p_array, p_item)
#define DECL_LIST_AT(p_array, p_idx) DYN_ARRAY_AT(PDecl*, p_array, p_idx)

static void
consume_token(struct PParser* p_parser)
{
  p_parser->prev_lookahead_end_loc = LOOKAHEAD_END_LOC;
  if (p_parser->token_run != nullptr) {
    p_parser->lookahead = (*p_parser->token_run)[p_parser->current_idx_in_token_run];
    p_parser->current_idx_in_token_run++;
    if (p_parser->current_idx_in_token_run >= p_parser->token_run->size()) {
      p_parser->token_run = nullptr;
      p_parser->current_idx_in_token_run = 0;
    }
  } else {
    lexer_next(p_parser->lexer, &p_parser->lookahead);
  }
}

static void
print_expect_tok_diag(struct PParser* p_parser, PTokenKind p_kind, PSourceLocation p_loc)
{
  PDiag* d = diag_at(P_DK_err_expected_tok, p_loc);
  diag_add_arg_tok_kind(d, p_kind);
  diag_add_arg_tok_kind(d, p_parser->lookahead.kind);
  diag_flush(d);
}

static bool
expect_token_with_loc(struct PParser* p_parser, PTokenKind p_kind, PSourceLocation p_loc)
{
  if (p_parser->lookahead.kind != p_kind) {
    print_expect_tok_diag(p_parser, p_kind, p_loc);
    consume_token(p_parser);
    return false;
  }

  consume_token(p_parser);
  return true;
}

static bool
expect_token(struct PParser* p_parser, PTokenKind p_kind)
{
  if (p_parser->lookahead.kind != p_kind) {
    print_expect_tok_diag(p_parser, p_kind, p_parser->prev_lookahead_end_loc);
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
skip_until_no_consume(struct PParser* p_parser, PTokenKind p_kind)
{
  if (LOOKAHEAD_IS(p_kind))
    return true;

  print_expect_tok_diag(p_parser, p_kind, p_parser->prev_lookahead_end_loc);

  while (!LOOKAHEAD_IS(p_kind) && !LOOKAHEAD_IS(P_TOK_EOF))
    consume_token(p_parser);

  return false;
}

// Skips tokens until one of p_kinds (or EOF) is found which is not consumed.
static void
skip_until_any(struct PParser* p_parser, PTokenKind* p_kinds, size_t p_kind_count)
{
  while (true) {
    if (LOOKAHEAD_IS(P_TOK_EOF))
      return;

    for (size_t i = 0; i < p_kind_count; ++i) {
      if (LOOKAHEAD_IS(p_kinds[i]))
        return;
    }

    consume_token(p_parser);
  }
}

// Wrapper around skip_until_any() that takes only one token kind.
static void
skip_until(struct PParser* p_parser, PTokenKind p_kind)
{
  skip_until_any(p_parser, &p_kind, 1);
}

void
p_parser_init(struct PParser* p_parser)
{
  assert(p_parser != nullptr);

  p_parser->token_run = nullptr;
  p_parser->current_idx_in_token_run = 0;

  p_parser->sema.current_scope_cache_idx = 0;
  p_parser->sema.current_scope = nullptr;
  memset(p_parser->sema.scope_cache, 0, sizeof(p_parser->sema.scope_cache));
}

void
p_parser_destroy(struct PParser* p_parser)
{
  assert(p_parser != nullptr);

  assert(p_parser->sema.current_scope == nullptr);

  for (size_t i = 0; i < P_MAX_SCOPE_CACHE; ++i) {
    if (p_parser->sema.scope_cache[i] != nullptr)
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
 *     IDENTIFIER
 */
static PType*
parse_type(struct PParser* p_parser)
{
  if (LOOKAHEAD_IS(P_TOK_STAR)) {
    consume_token(p_parser);
    return p_type_get_pointer(parse_type(p_parser));
  }
  if (LOOKAHEAD_IS(P_TOK_LPAREN)) {
    consume_token(p_parser);
    PType* sub_type = parse_type(p_parser);
    expect_token(p_parser, P_TOK_RPAREN);
    return p_type_get_paren(sub_type);
  }
  if (LOOKAHEAD_IS(P_TOK_IDENTIFIER)) {
    // TODO
    PType* type = nullptr;
    PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
    PIdentifierInfo* name = p_parser->lookahead.data.identifier;
    PSymbol* symbol = sema_lookup(&p_parser->sema, name);
    if (symbol == nullptr) {
      PDiag* d = diag_at(P_DK_err_use_undeclared_type, name_range.begin);
      diag_add_arg_ident(d, name);
      diag_add_source_range(d, name_range);
      diag_flush(d);
    } else {
      type = P_DECL_GET_TYPE(symbol->decl);
    }

    consume_token(p_parser);
    return type;
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
      return nullptr;
  }

  consume_token(p_parser);
  return type;
}

/*
 * param_decl:
 *     IDENTIFIER "=" expr
 *     IDENTIFIER type_specifier ("=" expr)?
 *
 * var_decl:
 *     IDENTIFIER "=" expr
 *     IDENTIFIER type_specifier ("=" expr)?
 *
 * type_specifier:
 *     ":" type
 */
static PDecl*
parse_param_or_var_decl(struct PParser* p_parser, bool p_is_param)
{
  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  if (!LOOKAHEAD_IS(P_TOK_IDENTIFIER)) {
    PDiag* d =
      diag_at(p_is_param ? P_DK_err_expected_param_decl : P_DK_err_expected_var_decl, p_parser->prev_lookahead_end_loc);
    diag_add_source_caret(d, p_parser->prev_lookahead_end_loc);
    diag_flush(d);
    if (p_is_param) {
      PTokenKind kinds[] = { P_TOK_COMMA, P_TOK_RPAREN };
      skip_until_any(p_parser, kinds, 2);
    } else {
      skip_until(p_parser, P_TOK_SEMI);
    }
    return nullptr;
  }

  PIdentifierInfo* name = name = p_parser->lookahead.data.identifier;

  expect_token(p_parser, P_TOK_IDENTIFIER);

  PType* type = nullptr;
  if (LOOKAHEAD_IS(P_TOK_COLON)) {
    consume_token(p_parser); // consume ':'
    type = parse_type(p_parser);
  }

  PAst* default_expr = nullptr;
  if (LOOKAHEAD_IS(P_TOK_EQUAL)) {
    consume_token(p_parser); // consume '='
    default_expr = parse_expr(p_parser);
  }

  if (p_is_param)
    return (PDecl*)sema_act_on_param_decl(&p_parser->sema, name_range, name, type, default_expr);
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
parse_param_or_var_list(struct PParser* p_parser, std::vector<PDecl*>& p_param_list, bool p_is_param)
{
  while (true) {
    PDecl* decl = parse_param_or_var_decl(p_parser, p_is_param);
    if (decl != nullptr)
      p_param_list.push_back(decl);

    if (LOOKAHEAD_IS(P_TOK_EOF) || (p_is_param ? LOOKAHEAD_IS(P_TOK_RPAREN) : LOOKAHEAD_IS(P_TOK_SEMI)))
      break;

    expect_token(p_parser, P_TOK_COMMA);
  }
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
  assert(LOOKAHEAD_IS(P_TOK_LBRACE));

  sema_push_scope(&p_parser->sema, P_SF_NONE);

  PSourceLocation lbrace_loc = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser); // consume '{'

  std::vector<PAst*> stmts;
  while (!LOOKAHEAD_IS(P_TOK_RBRACE) && !LOOKAHEAD_IS(P_TOK_EOF)) {
    PAst* stmt = parse_stmt(p_parser);
    stmts.push_back(stmt);
  }

  PSourceLocation rbrace_loc = LOOKAHEAD_BEGIN_LOC;
  expect_token(p_parser, P_TOK_RBRACE);

  PAstCompoundStmt* node =
    CREATE_NODE_EXTRA_SIZE(PAstCompoundStmt, sizeof(PAst*) * (stmts.size() - 1), P_AST_NODE_COMPOUND_STMT);
  node->stmt_count = stmts.size();
  memcpy(node->stmts, stmts.data(), sizeof(PAst*) * stmts.size());
  SET_NODE_LOC_RANGE(node, lbrace_loc, rbrace_loc + 1);
  sema_pop_scope(&p_parser->sema);
  return (PAst*)node;
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

  std::vector<PDecl*> var_decls;
  parse_param_or_var_list(p_parser, var_decls, /* is_param */ false);

  expect_token(p_parser, P_TOK_SEMI);

  PAstLetStmt* node = sema_act_on_let_stmt(&p_parser->sema, (PDeclVar**)var_decls.data(), var_decls.size());
  if (node == nullptr)
    return nullptr;

  SET_NODE_LOC_RANGE(node, loc_begin, p_parser->prev_lookahead_end_loc);
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
  if (node == nullptr)
    return nullptr;

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
  if (node == nullptr)
    return nullptr;

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

  PAst* ret_expr = nullptr;
  if (!LOOKAHEAD_IS(P_TOK_SEMI))
    ret_expr = parse_expr(p_parser);

  PSourceLocation loc_end = LOOKAHEAD_END_LOC;
  expect_token(p_parser, P_TOK_SEMI);

  PAstReturnStmt* node = sema_act_on_return_stmt(&p_parser->sema, loc_end, ret_expr);
  if (node == nullptr)
    return nullptr;

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
  PAst* else_stmt = nullptr;
  if (LOOKAHEAD_IS(P_TOK_KEY_else)) {
    consume_token(p_parser);

    if (LOOKAHEAD_IS(P_TOK_KEY_if)) {
      else_stmt = parse_if_stmt(p_parser);
    } else {
      else_stmt = parse_compound_stmt(p_parser);
    }

    if (else_stmt != nullptr)
      loc_end = P_AST_GET_SOURCE_RANGE(else_stmt).end;
  } else if (then_stmt != nullptr) {
    loc_end = P_AST_GET_SOURCE_RANGE(then_stmt).end;
  }

  PAstIfStmt* node = sema_act_on_if_stmt(&p_parser->sema, cond_expr, then_stmt, else_stmt);
  if (node == nullptr)
    return nullptr;

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

  sema_push_scope(&p_parser->sema, static_cast<PScopeFlags>(P_SF_BREAK | P_SF_CONTINUE));

  PAstLoopStmt* node = sema_act_on_loop_stmt(&p_parser->sema, p_parser->sema.current_scope);
  if (node == nullptr) {
    parse_compound_stmt(p_parser);
    sema_pop_scope(&p_parser->sema);
    return nullptr;
  }

  node->body_stmt = parse_compound_stmt(p_parser);
  if (node->body_stmt == nullptr) {
    sema_pop_scope(&p_parser->sema);
    return nullptr;
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

  sema_push_scope(&p_parser->sema, static_cast<PScopeFlags>(P_SF_BREAK | P_SF_CONTINUE));

  PAst* cond_expr = parse_expr(p_parser);

  PAstWhileStmt* node = sema_act_on_while_stmt(&p_parser->sema, cond_expr, p_parser->sema.current_scope);
  if (node == nullptr) {
    parse_compound_stmt(p_parser);
    sema_pop_scope(&p_parser->sema);
    return nullptr;
  }

  node->body_stmt = parse_compound_stmt(p_parser);
  if (node->body_stmt == nullptr) {
    sema_pop_scope(&p_parser->sema);
    return nullptr;
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
  if (node == nullptr)
    return nullptr;

  SET_NODE_LOC_RANGE(node, range.begin, range.end);
  return (PAst*)node;
}

static PType*
get_type_for_int_literal_suffix(PIntLiteralSuffix p_suffix_kind)
{
  switch (p_suffix_kind) {
    case P_ILS_NO_SUFFIX:
      return nullptr; // let semantic analyzer choose
    case P_ILS_I8:
      return p_type_get_i8();
    case P_ILS_I16:
      return p_type_get_i16();
    case P_ILS_I32:
      return p_type_get_i32();
    case P_ILS_I64:
      return p_type_get_i64();
    case P_ILS_U8:
      return p_type_get_u8();
    case P_ILS_U16:
      return p_type_get_u16();
    case P_ILS_U32:
      return p_type_get_u32();
    case P_ILS_U64:
      return p_type_get_u64();
    default:
      HEDLEY_UNREACHABLE_RETURN(nullptr);
  }
}

/*
 * int_literal:
 *     INT_LITERAL
 */
static PAst*
parse_int_literal(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_INT_LITERAL));

  PSourceRange range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };

  const char* literal_begin = p_parser->lookahead.data.literal.begin;
  const char* literal_end = p_parser->lookahead.data.literal.end;

  uintmax_t value;

  bool overflow;
  if (p_parser->lookahead.data.literal.int_radix == 10) {
    overflow = parse_dec_int_literal_token(literal_begin, literal_end, &value);
  } else if (p_parser->lookahead.data.literal.int_radix == 2) {
    overflow = parse_bin_int_literal_token(literal_begin, literal_end, &value);
  } else if (p_parser->lookahead.data.literal.int_radix == 8) {
    overflow = parse_oct_int_literal_token(literal_begin, literal_end, &value);
  }  else {
    // p_parser->lookahead.data.literal.int_radix == 16
    overflow = parse_hex_int_literal_token(literal_begin, literal_end, &value);
  }

  if (overflow) {
    PDiag* d = diag_at(P_DK_err_generic_int_literal_too_large, range.begin);
    diag_add_source_range(d, range);
    diag_flush(d);
    value = 0; // set a default value to recover
  }

  PType* type = get_type_for_int_literal_suffix(static_cast<PIntLiteralSuffix>(p_parser->lookahead.data.literal.suffix_kind));

  consume_token(p_parser);

  PAstIntLiteral* node = sema_act_on_int_literal(&p_parser->sema, range, type, value);
  assert(node != nullptr);

  SET_NODE_LOC_RANGE(node, range.begin, range.end);
  return (PAst*)node;
}

static PType*
get_type_for_float_literal_suffix(PFloatLiteralSuffix p_suffix_kind)
{
  switch (p_suffix_kind) {
    case P_FLS_NO_SUFFIX:
      return nullptr; // let semantic analyzer choose
    case P_FLS_F32:
      return p_type_get_f32();
    case P_FLS_F64:
      return p_type_get_f64();
    default:
      HEDLEY_UNREACHABLE_RETURN(nullptr);
  }
}

/*
 * float_literal:
 *     FLOAT_LITERAL
 */
static PAst*
parse_float_literal(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_FLOAT_LITERAL));

  PSourceRange range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };

  const char* literal_begin = p_parser->lookahead.data.literal.begin;
  const char* literal_end = p_parser->lookahead.data.literal.end;

  double value;
  if (parse_float_literal_token(literal_begin, literal_end, &value)) {
    PDiag* d = diag_at(P_DK_err_generic_float_literal_too_large, range.begin);
    diag_add_source_range(d, range);
    diag_flush(d);
    value = 0.0; // set a default value to recover
  }

  PType* type = get_type_for_float_literal_suffix(static_cast<PFloatLiteralSuffix>(p_parser->lookahead.data.literal.suffix_kind));

  consume_token(p_parser);

  PAstFloatLiteral* node = sema_act_on_float_literal(&p_parser->sema, range, type, value);
  assert(node != nullptr);

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

  PAst* sub_expr = nullptr;
  if (LOOKAHEAD_IS(P_TOK_RPAREN) || LOOKAHEAD_IS(P_TOK_EOF)) {
    PDiag* d = diag_at(P_DK_err_expected_expr, lparen_loc + 1);
    diag_add_source_caret(d, lparen_loc + 1);
    diag_flush(d);
  } else {
    sub_expr = parse_expr(p_parser);
  }

  PSourceLocation rparen_loc = LOOKAHEAD_BEGIN_LOC;
  expect_token(p_parser, P_TOK_RPAREN);

  PAstParenExpr* node = sema_act_on_paren_expr(&p_parser->sema, lparen_loc, rparen_loc, sub_expr);
  if (node == nullptr)
    return nullptr;

  SET_NODE_LOC_RANGE(node, lparen_loc, rparen_loc + 1);
  return (PAst*)node;
}

/*
 * decl_ref_expr:
 *     IDENTIFIER
 */
static PAst*
parse_decl_ref_expr(struct PParser* p_parser)
{
  assert(p_parser->lookahead.kind == P_TOK_IDENTIFIER);

  PSourceRange range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = p_parser->lookahead.data.identifier;
  consume_token(p_parser);

  PAstDeclRefExpr* node = sema_act_on_decl_ref_expr(&p_parser->sema, range, name);
  if (node == nullptr)
    return nullptr;

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
      return nullptr;
  }
}

/*
 * call_expr:
 *     postfix_expr "(" arg_list ")"
 *
 * arg_list:
 *     expr
 *     arg_list "," expr
 */
static PAst*
parse_call_expr(struct PParser* p_parser, PAst* p_callee)
{
  assert(LOOKAHEAD_IS(P_TOK_LPAREN));

  PSourceLocation lparen_loc = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser); /* consume '(' */

  /* Parse arguments: */
  std::vector<PAst*> args;
  while (!LOOKAHEAD_IS(P_TOK_RPAREN)) {
    PAst* arg = parse_expr(p_parser);
    args.push_back(arg);

    if (!LOOKAHEAD_IS(P_TOK_COMMA))
      break;

    consume_token(p_parser); /* consume ',' */
  }

  PSourceLocation rparen_loc = LOOKAHEAD_BEGIN_LOC;
  expect_token(p_parser, P_TOK_RPAREN);

  PAstCallExpr* node =
    sema_act_on_call_expr(&p_parser->sema, lparen_loc, rparen_loc, p_callee, (PAst**)args.data(), args.size());
  if (node == nullptr)
    return nullptr;

  SET_NODE_LOC_RANGE(node, P_AST_GET_SOURCE_RANGE(p_callee).begin, rparen_loc + 1);
  return (PAst*)node;
}

/*
 * member_expr:
 *     postfix_expr "." IDENTIFIER
 */
static PAst*
parse_member_expr(struct PParser* p_parser, PAst* p_base_expr)
{
  assert(LOOKAHEAD_IS(P_TOK_DOT));

  PSourceLocation dot_loc = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser); // consume '.'

  if (!LOOKAHEAD_IS(P_TOK_IDENTIFIER)) {
    unexpected_token(p_parser);
    return nullptr;
  }

  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = p_parser->lookahead.data.identifier;
  consume_token(p_parser); // consume identifier

  PAstMemberExpr* node = sema_act_on_member_expr(&p_parser->sema, dot_loc, p_base_expr, name_range, name);
  if (node == nullptr)
    return nullptr;

  SET_NODE_LOC_RANGE(node, P_AST_GET_SOURCE_RANGE(p_base_expr).begin, name_range.end);
  return (PAst*)node;
}

/*
 * postfix_expr:
 *     primary_expr
 *     call_expr
 *     member_expr
 */
static PAst*
parse_postfix_expr(struct PParser* p_parser)
{
  PAst* expr = parse_primary_expr(p_parser);
  while (true) {
    switch (p_parser->lookahead.kind) {
      case P_TOK_LPAREN: // call expr
        expr = parse_call_expr(p_parser, expr);
        break;
      case P_TOK_DOT: // member expr
        expr = parse_member_expr(p_parser, expr);
        break;
      default:
        return expr;
    }
  }
}

/*
 * postfix_expr:
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
      return parse_postfix_expr(p_parser);
  }

  PSourceLocation op_loc = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser); // consume opcode

  PAst* sub_expr = parse_unary_expr(p_parser);
  PAstUnaryExpr* node = sema_act_on_unary_expr(&p_parser->sema, opcode, op_loc, sub_expr);
  if (node == nullptr)
    return nullptr;

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
    if (node == nullptr)
      return nullptr;

    // FIXME: better end source location for cast expr instead of LOOKAHEAD_END_LOC
    //        probably use type->source_range.end when types have localizations.
    SET_NODE_LOC_RANGE(node, P_AST_GET_SOURCE_RANGE(sub_expr).begin, as_loc + 2);
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
    if (rhs == nullptr)
      return nullptr;

    /* If BinOp binds less tightly with RHS than the operator after RHS, let
     * the pending operator take RHS as its LHS. */
    int new_prec = get_binop_precedence(p_parser->lookahead.kind);
    if (tok_prec < new_prec) {
      rhs = parse_binary_expr(p_parser, rhs, tok_prec + 1);
      if (rhs == nullptr)
        return nullptr;
    }

    /* Merge lhs / rhs. */
    PAstBinaryExpr* new_lhs = sema_act_on_binary_expr(&p_parser->sema, binary_opcode, op_loc, p_lhs, rhs);
    if (new_lhs == nullptr)
      return nullptr;

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

static void
tokenize_func_body(struct PParser* p_parser, std::vector<PToken>& p_token_run)
{
  assert(LOOKAHEAD_IS(P_TOK_LBRACE));

  int depth = 1;
  do {
    p_token_run.push_back(p_parser->lookahead);
    consume_token(p_parser);
    if (LOOKAHEAD_IS(P_TOK_LBRACE)) {
      ++depth;
    } else if (LOOKAHEAD_IS(P_TOK_RBRACE) && --depth == 0) {
      break;
    }
  } while (!LOOKAHEAD_IS(P_TOK_EOF));

  p_token_run.push_back(p_parser->lookahead);
  expect_token(p_parser, P_TOK_RBRACE);
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

  PSourceLocation key_fn_end_loc = LOOKAHEAD_END_LOC;
  consume_token(p_parser);

  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = nullptr;
  if (LOOKAHEAD_IS(P_TOK_IDENTIFIER)) {
    name = p_parser->lookahead.data.identifier;
    consume_token(p_parser); // consume identifier
  } else {
    // Act as if there was an identifier but emit a diagnostic.
    print_expect_tok_diag(p_parser, P_TOK_IDENTIFIER, key_fn_end_loc);
  }

  std::vector<PDecl*> params;

  // Parse parameters
  expect_token(p_parser, P_TOK_LPAREN);

  sema_push_scope(&p_parser->sema, P_SF_FUNC_PARAMS);
  if (!LOOKAHEAD_IS(P_TOK_RPAREN))
    parse_param_or_var_list(p_parser, params, /* is_param */ true);
  sema_pop_scope(&p_parser->sema);

  PSourceLocation rparen_loc = LOOKAHEAD_BEGIN_LOC;
  if (LOOKAHEAD_IS(P_TOK_EOF)) {
    print_expect_tok_diag(p_parser, P_TOK_RPAREN, p_parser->prev_lookahead_end_loc);
    return nullptr;
  }

  expect_token(p_parser, P_TOK_RPAREN);

  // Parse return type specified (optional)
  PType* return_type = nullptr;
  if (LOOKAHEAD_IS(P_TOK_ARROW)) {
    consume_token(p_parser); // consume '->'
    return_type = parse_type(p_parser);
  }

  PDeclFunction* decl =
    sema_act_on_func_decl(&p_parser->sema, name_range, name, return_type, (PDeclParam**)params.data(), params.size());

  // Parse body
  if (!LOOKAHEAD_IS(P_TOK_LBRACE)) {
    PDiag* d = diag_at(P_DK_err_expected_func_body_after_func_decl, rparen_loc + 1);
    diag_add_source_caret(d, rparen_loc + 1);
    diag_flush(d);
    return (PDecl*)decl;
  }

  if (decl == nullptr) {
    // Skip body
    expect_token(p_parser, P_TOK_LBRACE);
    skip_until_no_error(p_parser, P_TOK_RBRACE);
    consume_token(p_parser); // consume '}'
    return (PDecl*)decl;
  }

  decl->lazy_body_token_run.clear();
  tokenize_func_body(p_parser, decl->lazy_body_token_run);

  return (PDecl*)decl;
}

static void
parse_lazy_func_body(struct PParser* p_parser, PDeclFunction* p_decl)
{
  assert(P_DECL_GET_KIND(p_decl) == P_DECL_FUNCTION);

  p_parser->token_run = &p_decl->lazy_body_token_run;
  consume_token(p_parser); // fill p_parser->lookahead with the content of token_run

  sema_begin_func_decl_body_parsing(&p_parser->sema, p_decl);
  PAst* body = parse_compound_stmt(p_parser);
  sema_end_func_decl_body_parsing(&p_parser->sema, p_decl);

  p_decl->body = body;
  p_decl->lazy_body_token_run.clear();
}

/*
 * struct_field_decl:
 *     IDENTIFIER type_specifier ";"
 */
static PDecl*
parse_struct_field_decl(struct PParser* p_parser)
{
  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = nullptr;
  if (LOOKAHEAD_IS(P_TOK_IDENTIFIER))
    name = p_parser->lookahead.data.identifier;

  if (!expect_token(p_parser, P_TOK_IDENTIFIER))
    return nullptr;

  expect_token(p_parser, P_TOK_COLON);
  PType* type = parse_type(p_parser);

  expect_token(p_parser, P_TOK_SEMI);
  return (PDecl*)sema_act_on_struct_field_decl(&p_parser->sema, name_range, name, type);
}

/*
 * struct_field_decls:
 *     struct_field_decl
 *     struct_field_decls struct_field_decl
 */
static void
parse_struct_field_decls(struct PParser* p_parser, std::vector<PDecl*>& p_fields)
{
  while (!LOOKAHEAD_IS(P_TOK_RBRACE)) {
    PDecl* field = parse_struct_field_decl(p_parser);
    if (field != nullptr)
      p_fields.push_back(field);
  }
}

/*
 * struct_decl:
 *     "struct" identifier "{" "}"
 */
static PDecl*
parse_struct_decl(struct PParser* p_parser)
{
  assert(LOOKAHEAD_IS(P_TOK_KEY_struct));

  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser); // consume 'struct'

  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = nullptr;
  if (LOOKAHEAD_IS(P_TOK_IDENTIFIER)) {
    name = p_parser->lookahead.data.identifier;
    consume_token(p_parser); // consume identifier
  } else {
    // Act as if there was an identifier but emit a diagnostic.
    print_expect_tok_diag(p_parser, P_TOK_IDENTIFIER, p_parser->prev_lookahead_end_loc);
  }

  std::vector<PDecl*> fields;

  expect_token(p_parser, P_TOK_LBRACE);
  parse_struct_field_decls(p_parser, fields);
  expect_token(p_parser, P_TOK_RBRACE);

  PDeclStruct* decl =
    sema_act_on_struct_decl(&p_parser->sema, name_range, name, (PDeclStructField**)fields.data(), fields.size());

  if (decl == nullptr)
    return nullptr;

  return (PDecl*)decl;
}

/*
 * top_level_decl:
 *     function_decl
 *     struct_decl
 */
static PDecl*
parse_top_level_decl(struct PParser* p_parser)
{
  switch (p_parser->lookahead.kind) {
    case P_TOK_KEY_fn:
      return parse_func_decl(p_parser);
    case P_TOK_KEY_struct:
      return parse_struct_decl(p_parser);
    default:
      unexpected_token(p_parser);
      return nullptr;
  }
}

/*
 * top_level_decls:
 *     top_level_decl
 *     top_level_decls top_level_decl
 */
static void
parse_top_level_decls(struct PParser* p_parser, std::vector<PDecl*>& p_decls)
{
  while (!LOOKAHEAD_IS(P_TOK_EOF)) {
    PDecl* decl = parse_top_level_decl(p_parser);
    if (decl != nullptr)
      p_decls.push_back(decl);
  }
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

  std::vector<PDecl*> decls;
  parse_top_level_decls(p_parser, decls);

  expect_token(p_parser, P_TOK_EOF);

  PAstTranslationUnit* node =
    CREATE_NODE_EXTRA_SIZE(PAstTranslationUnit, sizeof(PDecl*) * (decls.size() - 1), P_AST_NODE_TRANSLATION_UNIT);
  node->decl_count = decls.size();
  memcpy(node->decls, decls.data(), sizeof(PDecl*) * decls.size());

  for (size_t i = 0; i < node->decl_count; ++i) {
    if (P_DECL_GET_KIND(node->decls[i]) == P_DECL_FUNCTION)
      parse_lazy_func_body(p_parser, (PDeclFunction*)node->decls[i]);
  }

  sema_pop_scope(&p_parser->sema);
  SET_NODE_LOC_RANGE(node, loc_begin, LOOKAHEAD_END_LOC);
  return (PAst*)node;
}

PAst*
p_parse(struct PParser* p_parser)
{
  assert(p_parser != nullptr);

  // We do not call consume_token() because it sets p_parser->prev_lookahead_end_loc
  // to the end position of lookahead which is undefined here.
  p_parser->prev_lookahead_end_loc = 0;
  lexer_next(p_parser->lexer, &p_parser->lookahead);

  PAst* ast = parse_translation_unit(p_parser);
  return ast;
}
