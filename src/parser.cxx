#include "parser.hxx"

#include "literal_parser.hxx"
#include "scope.hxx"

#include "utils/diag.hxx"

#include <cassert>
#include <vector>

class PBalancedDelimiterTracker
{
public:
  PBalancedDelimiterTracker(PParser& p_parser, PTokenKind p_open_token)
    : m_parser(p_parser)
    , m_open_token(p_open_token)
  {
  }

  void consume_open()
  {
    assert(m_parser.lookahead(m_open_token));
    m_parser.consume_token();
  }

  void consume_close()
  {
    assert(m_parser.lookahead(get_close_token()));
    m_parser.consume_token();
  }

  bool try_consume_open()
  {
    m_open_location = m_parser.m_token.source_location;
    return m_parser.try_consume_token(m_open_token);
  }

  bool try_consume_close()
  {
    m_close_location = m_parser.m_token.source_location;
    return m_parser.try_consume_token(get_close_token());
  }

  bool expect_and_consume_open()
  {
    if (try_consume_open())
      return true;

    PDiag* d = diag_at(P_DK_err_expected_tok, m_open_location);
    diag_add_source_caret(d, m_open_location);
    diag_add_arg_tok_kind(d, m_open_token);
    diag_add_arg_tok_kind(d, m_parser.m_token.kind);
    diag_flush(d);
    return false;
  }

  bool expect_and_consume_close()
  {
    if (try_consume_close())
      return true;

    PDiag* d = diag_at(P_DK_err_expected_tok, m_close_location);
    diag_add_source_caret(d, m_close_location);
    diag_add_arg_tok_kind(d, get_close_token());
    diag_add_arg_tok_kind(d, m_parser.m_token.kind);
    diag_flush(d);
    return false;
  }

  [[nodiscard]] PSourceLocation get_open_location() const { return m_open_location; }

  [[nodiscard]] PSourceLocation get_close_location() const { return m_close_location; }

  [[nodiscard]] PSourceRange get_source_range() const { return { m_open_location, m_close_location + 1 }; }

private:
  [[nodiscard]] PTokenKind get_close_token()
  {
    switch (m_open_token) {
      case P_TOK_LPAREN:
        return P_TOK_RPAREN;
      case P_TOK_LBRACE:
        return P_TOK_RBRACE;
      case P_TOK_LSQUARE:
        return P_TOK_RSQUARE;
      default:
        assert(false && "not a open delimiter token");
        return P_TOK_EOF;
    }
  }

private:
  PParser& m_parser;
  PTokenKind m_open_token;
  PSourceLocation m_open_location;
  PSourceLocation m_close_location;
};

class PSourceRangeTracker
{
public:
  PSourceRangeTracker(PParser& p_parser)
    : m_parser(p_parser)
    , m_start(p_parser.m_token.source_location)
  {
  }

  [[nodiscard]] PSourceRange get_source_range() const { return { m_start, m_parser.m_prev_lookahead_end_loc }; }

private:
  PParser& m_parser;
  PSourceLocation m_start;
};

PParser::PParser(PContext& p_context, PLexer& p_lexer)
  : m_context(p_context)
  , m_lexer(p_lexer)
  , m_sema(p_context)
{
}

PParser::~PParser() {}

// type:
//   void
//   char
//   bool
//   i8
//   i16
//   i32
//   i64
//   u8
//   u16
//   u32
//   u64
//   f32
//   f64
//   "*" type
//   "(" type ")"
//   IDENTIFIER
PType*
PParser::parse_type()
{
  switch (m_token.kind) {
      // Pointer type:
    case P_TOK_STAR:
      consume_token();
      return m_context.get_pointer_ty(parse_type());

      // Parenthesized type:
    case P_TOK_LPAREN: {
      consume_token();
      PType* sub_type = parse_type();
      expect_token(P_TOK_RPAREN);
      return m_context.get_paren_ty(sub_type);
    }

    case P_TOK_IDENTIFIER: {
      auto parse_ident_info = parse_identifier([]() { assert(false && "unreachable"); });
      return m_sema.lookup_type(parse_ident_info);
    }

      // Builtin types:
    case P_TOK_KEY_void:
      consume_token();
      return m_context.get_void_ty();
    case P_TOK_KEY_char:
      consume_token();
      return m_context.get_char_ty();
    case P_TOK_KEY_bool:
      consume_token();
      return m_context.get_bool_ty();
    case P_TOK_KEY_i8:
      consume_token();
      return m_context.get_i8_ty();
    case P_TOK_KEY_i16:
      consume_token();
      return m_context.get_i16_ty();
    case P_TOK_KEY_i32:
      consume_token();
      return m_context.get_i32_ty();
    case P_TOK_KEY_i64:
      consume_token();
      return m_context.get_i64_ty();
    case P_TOK_KEY_u8:
      consume_token();
      return m_context.get_u8_ty();
    case P_TOK_KEY_u16:
      consume_token();
      return m_context.get_u16_ty();
    case P_TOK_KEY_u32:
      consume_token();
      return m_context.get_u32_ty();
    case P_TOK_KEY_u64:
      consume_token();
      return m_context.get_u64_ty();
    case P_TOK_KEY_f32:
      consume_token();
      return m_context.get_f32_ty();
    case P_TOK_KEY_f64:
      consume_token();
      return m_context.get_f64_ty();

    default:
      unexpected_token();
      return nullptr;
  }
}

// param_decl:
//     IDENTIFIER type_specifier
//
// type_specifier:
//     ":" type
PParamDecl*
PParser::parse_param_decl()
{
  PSourceRangeTracker range_tracker(*this);

  bool next_is_colon = lookahead(P_TOK_COLON);
  auto ident_parse_info = parse_identifier([this, next_is_colon]() {
    // If there is a chance that the user just forgot the name, let semantic
    // analyzer handle this error.
    if (next_is_colon)
      return;

    PDiag* d = diag_at(P_DK_err_param_decl_expected, m_token.source_location);
    diag_flush(d);
  });

  // No chance that it was just the name that was forgotten. Do not try to recover.
  if (ident_parse_info.ident == nullptr && !next_is_colon)
    return nullptr;

  PType* type = try_parse_type_specifier();

  auto* node = m_sema.act_on_param_decl(type, ident_parse_info, range_tracker.get_source_range());
  return node;
}

// var_decl:
//     IDENTIFIER "=" expr
//     IDENTIFIER type_specifier ("=" expr)?
//
// type_specifier:
//     ":" type
PVarDecl*
PParser::parse_var_decl()
{
  PSourceRangeTracker range_tracker(*this);

  bool next_is_colon_or_eq = lookahead(P_TOK_COLON) || lookahead(P_TOK_EQUAL);
  auto ident_parse_info = parse_identifier([this, next_is_colon_or_eq]() {
    // If there is a chance that the user just forgot the name, let semantic
    // analyzer handle this error.
    if (next_is_colon_or_eq)
      return;

    PDiag* d = diag_at(P_DK_err_var_decl_expected, m_token.source_location);
    diag_flush(d);
  });

  // No chance that it was just the name that was forgotten. Do not try to recover.
  if (ident_parse_info.ident == nullptr && !next_is_colon_or_eq)
    return nullptr;

  PType* type = try_parse_type_specifier();

  PAstExpr* default_expr = nullptr;
  if (lookahead(P_TOK_EQUAL)) {
    consume_token(); // consume '='
    default_expr = parse_expr();
  }

  return m_sema.act_on_var_decl(type, ident_parse_info, default_expr, range_tracker.get_source_range());
}

// param_decl_list:
//     param_decl
//     param_decl_list "," param_decl
std::vector<PParamDecl*>
PParser::parse_param_list()
{
  std::vector<PParamDecl*> params;
  while (true) {
    PParamDecl* decl = parse_param_decl();
    if (decl != nullptr)
      params.push_back(decl);

    if (lookahead(P_TOK_EOF) || lookahead(P_TOK_RPAREN))
      break;

    expect_token(P_TOK_COMMA);
  }

  return params;
}

// var_decl_list:
//     var_decl
//     var_decl_list "," var_decl
std::vector<PVarDecl*>
PParser::parse_var_list()
{
  std::vector<PVarDecl*> vars;
  while (true) {
    PVarDecl* decl = parse_var_decl();
    if (decl != nullptr)
      vars.push_back(decl);

    if (lookahead(P_TOK_EOF) || lookahead(P_TOK_SEMI))
      break;

    expect_token(P_TOK_COMMA);
  }

  return vars;
}

// compound_stmt:
//     "{" stmt_list "}"
//
// stmt_list:
//     stmt
//     stmt_list stmt
PAst*
PParser::parse_compound_stmt()
{
  assert(lookahead(P_TOK_LBRACE));

  m_sema.push_scope(P_SF_NONE);

  PBalancedDelimiterTracker delimiters(*this, P_TOK_LBRACE);
  delimiters.try_consume_open();

  std::vector<PAst*> stmts;
  while (!lookahead(P_TOK_RBRACE) && !lookahead(P_TOK_EOF)) {
    PAst* stmt = parse_stmt();
    stmts.push_back(stmt);
  }

  delimiters.expect_and_consume_close();

  auto* raw_stmts = m_context.alloc_object<PAst*>(stmts.size());
  std::copy(stmts.begin(), stmts.end(), raw_stmts);

  auto* node =
    m_context.new_object<PAstCompoundStmt>(PArrayView{ raw_stmts, stmts.size() }, delimiters.get_source_range());
  m_sema.pop_scope();
  return node;
}

// let_stmt:
//     "let" var_decl_list ";"
PAst*
PParser::parse_let_stmt()
{
  assert(lookahead(P_TOK_KEY_let));

  PSourceRangeTracker range_tracker(*this);
  consume_token(); // consume 'let'

  auto var_decls = parse_var_list();

  expect_token(P_TOK_SEMI);
  return m_sema.act_on_let_stmt(var_decls, range_tracker.get_source_range());
}

// break_stmt:
//     "break" ";"
PAst*
PParser::parse_break_stmt()
{
  assert(lookahead(P_TOK_KEY_break));

  PSourceRangeTracker range_tracker(*this);

  PSourceRange break_range = get_token_range();
  consume_token(); // consume 'break'
  expect_token(P_TOK_SEMI);

  return m_sema.act_on_break_stmt(range_tracker.get_source_range(), break_range);
}

// continue_stmt:
//     "continue" ";"
PAst*
PParser::parse_continue_stmt()
{
  assert(lookahead(P_TOK_KEY_continue));

  PSourceRangeTracker range_tracker(*this);

  PSourceRange continue_range = get_token_range();
  consume_token(); // consume 'continue'
  expect_token(P_TOK_SEMI);

  return m_sema.act_on_continue_stmt(range_tracker.get_source_range(), continue_range);
}

// return_stmt:
//     "return" ";"
//     "return" expr ";"
PAst*
PParser::parse_return_stmt()
{
  assert(lookahead(P_TOK_KEY_return));

  PSourceRangeTracker range_tracker(*this);
  consume_token(); // consume 'return'

  PAstExpr* ret_expr = nullptr;
  if (!lookahead(P_TOK_SEMI))
    ret_expr = parse_expr();

  expect_token(P_TOK_SEMI);
  return m_sema.act_on_return_stmt(ret_expr, range_tracker.get_source_range());
}

// if_stmt:
//     "if" expr compound_stmt
//     "if" expr compound_stmt "else" (compound_stmt | if_stmt)
PAst*
PParser::parse_if_stmt()
{
  assert(lookahead(P_TOK_KEY_if));

  PSourceRangeTracker range_tracker(*this);
  consume_token(); // consume 'if'

  PAstExpr* cond_expr = parse_expr();
  PAst* then_stmt = parse_compound_stmt();

  PAst* else_stmt = nullptr;
  if (lookahead(P_TOK_KEY_else)) {
    consume_token(); // consume 'else'

    if (lookahead(P_TOK_KEY_if)) {
      // This allows `if { ... } else if { ... } ...` syntax.
      else_stmt = parse_if_stmt();
    } else {
      else_stmt = parse_compound_stmt();
    }
  }

  auto* node = m_sema.act_on_if_stmt(cond_expr, then_stmt, else_stmt, range_tracker.get_source_range());
  return node;
}

// loop_stmt:
//     "loop" compound_stmt
PAst*
PParser::parse_loop_stmt()
{
  assert(lookahead(P_TOK_KEY_loop));

  PSourceRangeTracker range_tracker(*this);
  consume_token(); // consume 'loop'

  m_sema.act_before_loop_stmt_body();
  PAst* body = parse_compound_stmt();

  auto* node = m_sema.act_on_loop_stmt(body, range_tracker.get_source_range());
  return node;
}

// while_stmt:
//     "while" expr compound_stmt
PAst*
PParser::parse_while_stmt()
{
  assert(lookahead(P_TOK_KEY_while));

  PSourceRangeTracker range_tracker(*this);
  consume_token();

  PAstExpr* cond_expr = parse_expr();

  m_sema.act_before_while_stmt_body();
  PAst* body = parse_compound_stmt();

  auto* node = m_sema.act_on_while_stmt(cond_expr, body, range_tracker.get_source_range());
  return node;
}

// assert_stmt:
//     "assert" expr ";"
PAst*
PParser::parse_assert_stmt()
{
  assert(lookahead(P_TOK_KEY_assert));

  PSourceRangeTracker range_tracker(*this);
  consume_token(); // consume 'assert'

  PAstExpr* cond_expr = parse_expr();
  expect_token(P_TOK_SEMI);
  return m_sema.act_on_assert_stmt(cond_expr, range_tracker.get_source_range());
}

// expr_stmt:
//     expr ";"
PAst*
PParser::parse_expr_stmt()
{
  PAst* expr = parse_expr();
  expect_token(P_TOK_SEMI);
  return expr;
}

// stmt:
//     compound_stmt
//     let_stmt
//     break_stmt
//     continue_stmt
//     return_stmt
//     if_stmt
//     while_stmt
//     assert_stmt
//     loop_stmt
//     expr_stmt
PAst*
PParser::parse_stmt()
{
  switch (m_token.kind) {
    case P_TOK_LBRACE:
      return parse_compound_stmt();
    case P_TOK_KEY_let:
      return parse_let_stmt();
    case P_TOK_KEY_break:
      return parse_break_stmt();
    case P_TOK_KEY_continue:
      return parse_continue_stmt();
    case P_TOK_KEY_return:
      return parse_return_stmt();
    case P_TOK_KEY_if:
      return parse_if_stmt();
    case P_TOK_KEY_while:
      return parse_while_stmt();
    case P_TOK_KEY_assert:
      return parse_assert_stmt();
    case P_TOK_KEY_loop:
      return parse_loop_stmt();
    default:
      return parse_expr_stmt();
  }
}

// bool_lit:
//     "true"
//     "false"
PAstExpr*
PParser::parse_bool_lit()
{
  assert(lookahead(P_TOK_KEY_true) || lookahead(P_TOK_KEY_false));

  PSourceRange range = get_token_range();
  const bool value = lookahead(P_TOK_KEY_true);
  consume_token();
  return m_sema.act_on_bool_literal(value, range);
}

// int_lit:
//     INT_LITERAL
PAstExpr*
PParser::parse_int_lit()
{
  assert(lookahead(P_TOK_INT_LITERAL));

  PSourceRange range = get_token_range();

  uintmax_t value;
  const bool overflow = parse_int_literal_token(
    m_token.data.literal.begin, m_token.data.literal.end, m_token.data.literal.int_radix, value);
  if (overflow) {
    PDiag* d = diag_at(P_DK_err_generic_int_literal_too_large, range.begin);
    diag_add_source_range(d, range);
    diag_flush(d);
    value = 0; // set a default value to recover
  }

  const auto suffix = static_cast<PIntLiteralSuffix>(m_token.data.literal.suffix_kind);
  consume_token();
  return m_sema.act_on_int_literal(value, suffix, range);
}

// float_lit:
//     FLOAT_LITERAL
PAstExpr*
PParser::parse_float_lit()
{
  assert(lookahead(P_TOK_FLOAT_LITERAL));

  PSourceRange range = get_token_range();

  double value;
  const bool overflow = parse_float_literal_token(m_token.data.literal.begin, m_token.data.literal.end, value);
  if (overflow) {
    PDiag* d = diag_at(P_DK_err_generic_float_literal_too_large, range.begin);
    diag_add_source_range(d, range);
    diag_flush(d);
    value = 0.0; // set a default value to recover
  }

  const auto suffix = static_cast<PFloatLiteralSuffix>(m_token.data.literal.suffix_kind);
  consume_token();
  return m_sema.act_on_float_literal(value, suffix, range);
}

// paren_expr:
//     "(" expr ")"
PAstExpr*
PParser::parse_paren_expr()
{
  PBalancedDelimiterTracker delimiters(*this, P_TOK_LPAREN);
  delimiters.consume_open();

  PAstExpr* sub_expr = nullptr;
  if (lookahead(P_TOK_RPAREN) || lookahead(P_TOK_EOF)) {
    PDiag* d = diag_at(P_DK_err_expected_expr, m_token.source_location);
    diag_add_source_caret(d, m_token.source_location);
    diag_flush(d);
  } else {
    sub_expr = parse_expr();
  }

  delimiters.expect_and_consume_close();
  return m_sema.act_on_paren_expr(sub_expr, delimiters.get_source_range());
}

// struct_field_expr:
//     IDENTIFIER
//     IDENTIFIER ":" expr
PAstStructFieldExpr*
PParser::parse_struct_field_expr(PStructDecl* p_struct_decl)
{
  auto ident_parse_info = parse_identifier([this]() {
    PDiag* d = diag_at(P_DK_err_expected_struct_field_decl, m_token.source_location);
    diag_add_source_caret(d, m_token.source_location);
    diag_flush(d);
  });

  if (ident_parse_info.ident == nullptr)
    return nullptr;

  PAstExpr* expr = nullptr;
  if (lookahead(P_TOK_COLON)) {
    consume_token(); // consume ':'
    expr = parse_expr();
  }

  return m_sema.act_on_struct_field_expr(p_struct_decl, ident_parse_info, expr);
}

// struct_expr:
//     IDENTIFIER "{"
//     "}"
PAstExpr*
PParser::parse_struct_expr(PLocalizedIdentifierInfo p_name)
{
  auto* struct_decl = m_sema.resolve_struct_expr_name(p_name);

  PBalancedDelimiterTracker delimiters(*this, P_TOK_LBRACE);
  delimiters.consume_open();

  std::vector<PAstStructFieldExpr*> fields;
  while (!lookahead(P_TOK_EOF) && !lookahead(P_TOK_RBRACE)) {
    PAstStructFieldExpr* field = parse_struct_field_expr(struct_decl);
    if (field != nullptr)
      fields.push_back(field);

    if (try_consume_token(P_TOK_COMMA))
      continue;

    // On other languages ';' is used instead of ',' to separate the struct
    // fields.
    if (lookahead(P_TOK_SEMI)) {
      PDiag* d = diag_at(P_DK_err_unexpected_tok_with_hint, m_token.source_location);
      diag_add_source_caret(d, m_token.source_location);
      diag_add_arg_tok_kind(d, P_TOK_SEMI);
      diag_add_arg_tok_kind(d, P_TOK_COMMA);
      diag_flush(d);

      // Act as if it were a ','.
      consume_token();
      continue;
    }

    if (lookahead(P_TOK_RBRACE))
      break;

    expect_token(P_TOK_COMMA);
    // Skip until '}' or ','.
    while (!lookahead(P_TOK_RBRACE) && !lookahead(P_TOK_COMMA))
      consume_token();
    // If we stopped at ',', eat it.
    try_consume_token(P_TOK_COMMA);
  }

  delimiters.expect_and_consume_close();
  return m_sema.act_on_struct_expr(
    struct_decl, fields, PSourceRange{ p_name.range.begin, delimiters.get_close_location() + 1 });
}

// decl_ref_expr:
//     IDENTIFIER
PAstExpr*
PParser::parse_decl_ref()
{
  assert(lookahead(P_TOK_IDENTIFIER));

  PLocalizedIdentifierInfo name = { m_token.data.identifier, get_token_range() };
  consume_token();

#if 0
  if (lookahead(P_TOK_LBRACE))
    return parse_struct_expr(name);
#endif

  return m_sema.act_on_decl_ref_expr(name.ident, name.range);
}

// primary_expr:
//     bool_lit
//     int_lit
//     float_lit
//     paren_expr
//     decl_ref_expr
PAstExpr*
PParser::parse_primary_expr()
{
  switch (m_token.kind) {
    case P_TOK_INT_LITERAL:
      return parse_int_lit();
    case P_TOK_FLOAT_LITERAL:
      return parse_float_lit();
    case P_TOK_KEY_true:
    case P_TOK_KEY_false:
      return parse_bool_lit();
    case P_TOK_LPAREN:
      return parse_paren_expr();
    case P_TOK_IDENTIFIER:
      return parse_decl_ref();
    default:
      unexpected_token();
      return nullptr;
  }
}

// call_expr:
//     postfix_expr "(" arg_list ")"
//
// arg_list:
//     expr
//     arg_list "," expr
PAstExpr*
PParser::parse_call_expr(PAstExpr* p_callee)
{
  PBalancedDelimiterTracker delimiters(*this, P_TOK_LPAREN);
  delimiters.consume_open();

  // Parse arguments:
  std::vector<PAstExpr*> args;
  while (!lookahead(P_TOK_RPAREN)) {
    PAstExpr* arg = parse_expr();
    args.push_back(arg);

    if (!lookahead(P_TOK_COMMA))
      break;

    consume_token(); // consume ','
  }

  delimiters.expect_and_consume_close();

  const auto range = PSourceRange{ p_callee->get_source_range().begin, delimiters.get_close_location() + 1 };
  auto* node = m_sema.act_on_call_expr(p_callee, args, range, delimiters.get_open_location());
  return node;
}

// member_expr:
//     postfix_expr "." IDENTIFIER
PAstExpr*
PParser::parse_member_expr(PAstExpr* p_base_expr)
{
  assert(lookahead(P_TOK_DOT));

  PSourceLocation dot_loc = m_token.source_location;
  consume_token(); // consume '.'

  if (!lookahead(P_TOK_IDENTIFIER)) {
    unexpected_token();
    return nullptr;
  }

  auto ident_parse_info = parse_identifier([] { assert(false && "unreachable"); });

  const auto range = PSourceRange{ p_base_expr->get_source_range().begin, m_prev_lookahead_end_loc };
  return m_sema.act_on_member_expr(p_base_expr, ident_parse_info, range, dot_loc);
}

// postfix_expr:
//     primary_expr
//     call_expr
//     member_expr
PAstExpr*
PParser::parse_postfix_expr()
{
  PAstExpr* expr = parse_primary_expr();
  while (true) {
    switch (m_token.kind) {
      case P_TOK_LPAREN: // call expr
        expr = parse_call_expr(expr);
        break;
      case P_TOK_DOT: // member expr
        expr = parse_member_expr(expr);
        break;
      default:
        return expr;
    }
  }
}

// unary_expr:
//     postfix_expr
//     "-" unary_expr
//     "!" unary_expr
//     "&" unary_expr
//     "*" deref_expr
PAstExpr*
PParser::parse_unary_expr()
{
  PSourceRangeTracker range_tracker(*this);

  PAstUnaryOp opcode;
  switch (m_token.kind) {
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
      return parse_postfix_expr();
  }

  consume_token(); // consume operator

  PAstExpr* sub_expr = parse_unary_expr();
  return m_sema.act_on_unary_expr(sub_expr, opcode, range_tracker.get_source_range());
}

// cast_expr:
//     unary_expr
//     unary_expr "as" type
PAstExpr*
PParser::parse_cast_expr()
{
  PSourceRangeTracker range_tracker(*this);
  PAstExpr* sub_expr = parse_unary_expr();

  if (lookahead(P_TOK_KEY_as)) {
    PSourceLocation as_loc = m_token.source_location;
    consume_token(); // consume 'as'

    PType* target_ty = parse_type();
    return m_sema.act_on_cast_expr(sub_expr, target_ty, range_tracker.get_source_range(), as_loc);
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
      assert(false && "unreachable");
      return P_BINARY_ADD;
  }
}

PAstExpr*
PParser::parse_binary_expr(PAstExpr* p_lhs, int p_expr_prec)
{
  while (true) {
    int tok_prec = get_binop_precedence(m_token.kind);

    /* If this is a binop that binds at least as tightly as the current
     * binary_op, consume_token it, otherwise we are done. */
    if (tok_prec < p_expr_prec)
      return p_lhs;

    /* Okay, we know this is a binary_op. */
    const PAstBinaryOp binary_opcode = get_binop_from_tok_kind(m_token.kind);
    const PSourceLocation op_loc = m_token.source_location;
    consume_token();

    /* Parse the primary expression after the binary operator. */
    PAstExpr* rhs = parse_cast_expr();
    if (rhs == nullptr)
      return nullptr;

    /* If BinOp binds less tightly with RHS than the operator after RHS, let
     * the pending operator take RHS as its LHS. */
    int new_prec = get_binop_precedence(m_token.kind);
    if (tok_prec < new_prec) {
      rhs = parse_binary_expr(rhs, tok_prec + 1);
      if (rhs == nullptr)
        return nullptr;
    }

    /* Merge lhs / rhs. */
    const auto range = PSourceRange{ p_lhs->get_source_range().begin, rhs->get_source_range().end };
    PAstBinaryExpr* new_lhs = m_sema.act_on_binary_expr(p_lhs, rhs, binary_opcode, range, op_loc);
    p_lhs = new_lhs;
  }
}

PAstExpr*
PParser::parse_expr()
{
  PAstExpr* lhs = parse_cast_expr();
  return parse_binary_expr(lhs, 0);
}

// func_decl:
//     extern_specifier? "fn" identifier "(" param_decl_list? ")" func_type_specifier? ";"
//     extern_specifier? "fn" identifier "(" param_decl_list? ")" func_type_specifier? compound_stmt
//
// func_type_specifier:
//     "->" type
//
// extern_specifier: ; this is parsed by parse_extern_decl()
//     "extern" STRING_LITERAL?
PFunctionDecl*
PParser::parse_func_decl(bool p_is_extern)
{
  assert(lookahead(P_TOK_KEY_fn));

  PSourceRangeTracker range_tracker(*this);
  consume_token(); // consume 'fn'

  bool next_is_lparen = lookahead(P_TOK_LPAREN);
  auto ident_parse_info = parse_identifier([this]() {
    PDiag* d = diag_at(P_DK_err_expected_tok, m_token.source_location);
    diag_add_source_caret(d, m_token.source_location);
    diag_add_arg_tok_kind(d, P_TOK_IDENTIFIER);
    diag_add_arg_tok_kind(d, m_token.kind);
    diag_flush(d);
  });

  if (ident_parse_info.ident == nullptr && !next_is_lparen)
    return nullptr;

  std::vector<PParamDecl*> params;

  // Parse parameters
  PBalancedDelimiterTracker delimiters(*this, P_TOK_LPAREN);
  if (!delimiters.expect_and_consume_open())
    return nullptr;

  m_sema.push_scope(P_SF_FUNC_PARAMS);
  if (!lookahead(P_TOK_RPAREN))
    params = parse_param_list();
  m_sema.pop_scope();

  if (!delimiters.expect_and_consume_close())
    return nullptr;

  // Parse return type specified (optional)
  PType* ret_ty = nullptr;
  if (lookahead(P_TOK_ARROW)) {
    consume_token(); // consume '->'
    ret_ty = parse_type();
  }

  PFunctionDecl* decl = m_sema.act_on_func_decl(ident_parse_info, ret_ty, params, delimiters.get_open_location());
  decl->source_range = range_tracker.get_source_range();

  if (p_is_extern && !lookahead(P_TOK_LBRACE)) {
    // External function declaration may not have a body.
    expect_token(P_TOK_SEMI);
  } else {
    // Parse body
    if (!lookahead(P_TOK_LBRACE)) {
      PDiag* d = diag_at(P_DK_err_expected_func_body_after_func_decl, delimiters.get_close_location() + 1);
      diag_add_source_caret(d, delimiters.get_close_location() + 1);
      diag_flush(d);
      return decl;
    }

    m_sema.begin_func_decl_analysis(decl);
    decl->body = parse_compound_stmt();
    m_sema.end_func_decl_analysis();

    decl->source_range = range_tracker.get_source_range();
  }

  return decl;
}

PDecl*
PParser::parse_extern_decl()
{
  assert(lookahead(P_TOK_KEY_extern));

  consume_token(); // consume 'extern'

  std::string abi;
  PSourceRange abi_range;
  bool has_abi = lookahead(P_TOK_STRING_LITERAL);
  if (has_abi) {
    abi = parse_string_literal_token(m_token.data.literal.begin, m_token.data.literal.end);
    abi_range = get_token_range();
    consume_token();
  }

  if (lookahead(P_TOK_KEY_fn)) {
    char* copied_abi = m_context.alloc_object<char>(abi.size() + 1);
    std::copy(abi.begin(), abi.end(), copied_abi);
    copied_abi[abi.size()] = '\0';

    PFunctionDecl* func_decl = parse_func_decl(true);
    func_decl->set_extern(true);
    if (has_abi) {
      func_decl->set_abi(std::string_view(copied_abi, abi.size()));
      m_sema.check_func_abi(func_decl->get_abi(), abi_range);
    }

    return func_decl;
  } else {
    return nullptr;
  }
}

// struct_field_decl:
//     IDENTIFIER type_specifier
PStructFieldDecl*
PParser::parse_struct_field_decl()
{
  PSourceRangeTracker range_tracker(*this);

  auto ident_parse_info = parse_identifier([this]() {
    PDiag* d = diag_at(P_DK_err_expected_struct_field_decl, m_token.source_location);
    diag_add_source_caret(d, m_token.source_location);
    diag_flush(d);
  });

  if (ident_parse_info.ident == nullptr)
    return nullptr;

  expect_token(P_TOK_COLON);
  PType* type = parse_type();

  return m_sema.act_on_struct_field_decl(type, ident_parse_info, range_tracker.get_source_range());
}

// struct_decl:
//     "struct" identifier "{" struct_field_decls "}"
//
// struct_field_decls:
//     struct_field_decl (',' struct_field_decl)* ','?
PStructDecl*
PParser::parse_struct_decl()
{
  assert(lookahead(P_TOK_KEY_struct));

  PSourceRangeTracker range_tracker(*this);
  consume_token(); // consume 'struct'

  auto ident_parse_info = parse_identifier([this]() {
    PDiag* d = diag_at(P_DK_err_expected_tok, m_token.source_location);
    diag_add_source_caret(d, m_token.source_location);
    diag_add_arg_tok_kind(d, P_TOK_IDENTIFIER);
    diag_add_arg_tok_kind(d, m_token.kind);
    diag_flush(d);
  });

  PBalancedDelimiterTracker delimiters(*this, P_TOK_LBRACE);
  if (!delimiters.expect_and_consume_open())
    return nullptr;

  std::vector<PStructFieldDecl*> fields;
  while (!lookahead(P_TOK_EOF) && !lookahead(P_TOK_RBRACE)) {
    PStructFieldDecl* field = parse_struct_field_decl();
    if (field != nullptr)
      fields.push_back(field);

    if (try_consume_token(P_TOK_COMMA))
      continue;

    // On other languages ';' is used instead of ',' to separate the struct
    // fields.
    if (lookahead(P_TOK_SEMI)) {
      PDiag* d = diag_at(P_DK_err_unexpected_tok_with_hint, m_token.source_location);
      diag_add_source_caret(d, m_token.source_location);
      diag_add_arg_tok_kind(d, P_TOK_SEMI);
      diag_add_arg_tok_kind(d, P_TOK_COMMA);
      diag_flush(d);

      // Act as if it were a ','.
      consume_token();
      continue;
    }

    if (lookahead(P_TOK_RBRACE))
      break;

    expect_token(P_TOK_COMMA);
    // Skip until '}' or ','.
    while (!lookahead(P_TOK_RBRACE) && !lookahead(P_TOK_COMMA))
      consume_token();
    // If we stopped at ',', eat it.
    try_consume_token(P_TOK_COMMA);
  }

  delimiters.expect_and_consume_close();
  return m_sema.act_on_struct_decl(ident_parse_info, fields, range_tracker.get_source_range());
}

// top_level_decl:
//     function_decl
//     struct_decl
PDecl*
PParser::parse_top_level_decl()
{
  switch (m_token.kind) {
    case P_TOK_KEY_extern:
      return parse_extern_decl();
    case P_TOK_KEY_fn:
      return parse_func_decl();
    case P_TOK_KEY_struct:
      return parse_struct_decl();
    default:
      unexpected_token();
      return nullptr;
  }
}

// translation_unit:
//     top_level_decl*
PAstTranslationUnit*
PParser::parse_translation_unit()
{
  PSourceRangeTracker range_tracker(*this);
  m_sema.push_scope(P_SF_NONE);

  std::vector<PDecl*> decls;
  while (!lookahead(P_TOK_EOF)) {
    PDecl* decl = parse_top_level_decl();
    if (decl != nullptr)
      decls.push_back(decl);
  }

  expect_token(P_TOK_EOF);

  auto* node = m_sema.act_on_translation_unit(decls);
  node->set_source_range(range_tracker.get_source_range());
  node->p_src_file = m_lexer.source_file;
  m_sema.pop_scope();
  return node;
}

PAstTranslationUnit*
PParser::parse()
{
  consume_token();
  m_prev_lookahead_end_loc = 0;
  return parse_translation_unit();
}

PAst*
PParser::parse_standalone_stmt()
{
  consume_token();
  m_prev_lookahead_end_loc = 0;
  return parse_stmt();
}

PAstExpr*
PParser::parse_standalone_expr()
{
  consume_token();
  m_prev_lookahead_end_loc = 0;
  return parse_expr();
}

PType*
PParser::try_parse_type_specifier()
{
  if (!try_consume_token(P_TOK_COLON))
    return nullptr;

  return parse_type();
}

void
PParser::consume_token()
{
  m_prev_lookahead_end_loc = m_token.source_location + m_token.token_length;
  m_lexer.tokenize(m_token);
}

bool
PParser::try_consume_token(PTokenKind p_kind)
{
  if (!lookahead(p_kind))
    return false;

  consume_token();
  return true;
}

bool
PParser::expect_token(PTokenKind p_kind)
{
  if (m_token.kind != p_kind) {
    print_expect_tok_diag(p_kind, m_prev_lookahead_end_loc);
    consume_token();
    return false;
  }

  consume_token();
  return true;
}

void
PParser::unexpected_token()
{
  PDiag* d = diag_at(P_DK_err_unexpected_tok, m_token.source_location);
  diag_add_arg_tok_kind(d, m_token.kind);
  diag_flush(d);
  consume_token();
}

void
PParser::print_expect_tok_diag(PTokenKind p_kind, PSourceLocation p_loc)
{
  PDiag* d = diag_at(P_DK_err_expected_tok, p_loc);
  diag_add_arg_tok_kind(d, p_kind);
  diag_add_arg_tok_kind(d, m_token.kind);
  diag_flush(d);
}
