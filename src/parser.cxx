#include "parser.hxx"

#include "literal_parser.hxx"
#include "scope.hxx"

#include "utils/diag.hxx"

#include <cassert>
#include <cstring>
#include <vector>

#define LOOKAHEAD_BEGIN_LOC ((p_parser)->m_token.source_location)
#define LOOKAHEAD_END_LOC ((p_parser)->m_token.source_location + (p_parser)->m_token.token_length)

static void
consume_token(PParser* p_parser)
{
  p_parser->prev_lookahead_end_loc = LOOKAHEAD_END_LOC;
  lexer_next(&p_parser->lexer, &p_parser->m_token);
}

static void
print_expect_tok_diag(PParser* p_parser, PTokenKind p_kind, PSourceLocation p_loc)
{
  PDiag* d = diag_at(P_DK_err_expected_tok, p_loc);
  diag_add_arg_tok_kind(d, p_kind);
  diag_add_arg_tok_kind(d, p_parser->m_token.kind);
  diag_flush(d);
}

static bool
expect_token(PParser* p_parser, PTokenKind p_kind)
{
  if (p_parser->m_token.kind != p_kind) {
    print_expect_tok_diag(p_parser, p_kind, p_parser->prev_lookahead_end_loc);
    consume_token(p_parser);
    return false;
  }

  consume_token(p_parser);
  return true;
}

static void
unexpected_token(PParser* p_parser)
{
  PDiag* d = diag_at(P_DK_err_unexpected_tok, LOOKAHEAD_BEGIN_LOC);
  diag_add_arg_tok_kind(d, p_parser->m_token.kind);
  diag_flush(d);
  consume_token(p_parser);
}

PParser::PParser(PContext& p_context, PLexer& p_lexer)
  : context(p_context)
  , lexer(p_lexer)
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
      consume();
      return context.get_pointer_ty(parse_type());

      // Parenthesized type:
    case P_TOK_LPAREN: {
      consume();
      PType* sub_type = parse_type();
      expect_token(this, P_TOK_RPAREN);
      return context.get_paren_ty(sub_type);
    }

    case P_TOK_IDENTIFIER: {
      // TODO
      PType* type = nullptr;
      const PSourceRange name_range = get_token_range();
      PIdentifierInfo* name = m_token.data.identifier;
      PSymbol* symbol = m_sema.lookup(name);
      if (symbol == nullptr) {
        PDiag* d = diag_at(P_DK_err_use_undeclared_type, name_range.begin);
        diag_add_arg_ident(d, name);
        diag_add_source_range(d, name_range);
        diag_flush(d);
      } else {
        type = P_DECL_GET_TYPE(symbol->decl);
      }

      consume();
      return type;
    }

      // Builtin types:
    case P_TOK_KEY_void:
      consume();
      return context.get_void_ty();
    case P_TOK_KEY_char:
      consume();
      return context.get_char_ty();
    case P_TOK_KEY_bool:
      consume();
      return context.get_bool_ty();
    case P_TOK_KEY_i8:
      consume();
      return context.get_i8_ty();
    case P_TOK_KEY_i16:
      consume();
      return context.get_i16_ty();
    case P_TOK_KEY_i32:
      consume();
      return context.get_i32_ty();
    case P_TOK_KEY_i64:
      consume();
      return context.get_i64_ty();
    case P_TOK_KEY_u8:
      consume();
      return context.get_u8_ty();
    case P_TOK_KEY_u16:
      consume();
      return context.get_u16_ty();
    case P_TOK_KEY_u32:
      consume();
      return context.get_u32_ty();
    case P_TOK_KEY_u64:
      consume();
      return context.get_u64_ty();
    case P_TOK_KEY_f32:
      consume();
      return context.get_f32_ty();
    case P_TOK_KEY_f64:
      consume();
      return context.get_f64_ty();
    default:
      unexpected_token(this);
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
  PIdentifierInfo* name = nullptr;
  const PSourceRange name_range = get_token_range();
  if (lookahead(P_TOK_IDENTIFIER)) {
    name = m_token.data.identifier;
    consume();
  } else if (!lookahead(P_TOK_COLON)) {
    PDiag* d = diag_at(P_DK_err_param_decl_expected, prev_lookahead_end_loc);
    diag_flush(d);
    return nullptr;
  }

  PType* type = nullptr;
  if (lookahead(P_TOK_COLON)) {
    consume(); // consume ':'
    type = parse_type();
  }

  auto* node = m_sema.act_on_param_decl(type, name, { name_range.begin, prev_lookahead_end_loc }, name_range);
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
  PIdentifierInfo* name = nullptr;
  const PSourceRange name_range = get_token_range();
  if (lookahead(P_TOK_IDENTIFIER)) {
    name = m_token.data.identifier;
    consume(); // consume identifier
  } else if (!(lookahead(P_TOK_COLON) || lookahead(P_TOK_EQUAL))) {
    PDiag* d = diag_at(P_DK_err_var_decl_expected, prev_lookahead_end_loc);
    diag_flush(d);
    return nullptr;
  }

  PType* type = nullptr;
  if (lookahead(P_TOK_COLON)) {
    consume(); // consume ':'
    type = parse_type();
  }

  PAstExpr* default_expr = nullptr;
  if (lookahead(P_TOK_EQUAL)) {
    consume(); // consume '='
    default_expr = parse_expr();
  }

  return m_sema.act_on_var_decl(type, name, default_expr, name_range);
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

    expect_token(this, P_TOK_COMMA);
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

    expect_token(this, P_TOK_COMMA);
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

  PSourceLocation lbrace_loc = m_token.source_location;
  consume(); // consume '{'

  std::vector<PAst*> stmts;
  while (!lookahead(P_TOK_RBRACE) && !lookahead(P_TOK_EOF)) {
    PAst* stmt = parse_stmt();
    stmts.push_back(stmt);
  }

  PSourceLocation rbrace_loc = m_token.source_location;
  expect_token(this, P_TOK_RBRACE);

  auto* raw_stmts = context.alloc_object<PAst*>(stmts.size());
  std::copy(stmts.begin(), stmts.end(), raw_stmts);

  auto* node = context.new_object<PAstCompoundStmt>(raw_stmts, stmts.size(), PSourceRange{ lbrace_loc, rbrace_loc + 1 });
  m_sema.pop_scope();
  return node;
}

// let_stmt:
//     "let" var_decl_list ";"
PAst*
PParser::parse_let_stmt()
{
  assert(lookahead(P_TOK_KEY_let));

  PSourceLocation loc_begin = m_token.source_location;
  consume();

  auto var_decls = parse_var_list();

  expect_token(this, P_TOK_SEMI);
  return m_sema.act_on_let_stmt(var_decls, PSourceRange{ loc_begin, prev_lookahead_end_loc });
}

// break_stmt:
//     "break" ";"
PAst*
PParser::parse_break_stmt()
{
  assert(lookahead(P_TOK_KEY_break));

  PSourceRange break_range = get_token_range();
  consume();

  PSourceLocation loc_end = m_token.source_location + m_token.token_length;
  expect_token(this, P_TOK_SEMI);

  return m_sema.act_on_break_stmt({ break_range.begin, loc_end }, break_range);
}

// continue_stmt:
//     "continue" ";"
PAst*
PParser::parse_continue_stmt()
{
  assert(lookahead(P_TOK_KEY_continue));

  PSourceRange continue_range = get_token_range();
  consume();

  PSourceLocation loc_end = m_token.source_location + m_token.token_length;
  expect_token(this, P_TOK_SEMI);

  return m_sema.act_on_continue_stmt({ continue_range.begin, loc_end }, continue_range);
}

// return_stmt:
//     "return" ";"
//     "return" expr ";"
PAst*
PParser::parse_return_stmt()
{
  assert(lookahead(P_TOK_KEY_return));

  PSourceLocation loc_begin = m_token.source_location;
  consume();

  PAstExpr* ret_expr = nullptr;
  if (!lookahead(P_TOK_SEMI))
    ret_expr = parse_expr();

  PSourceLocation semi_loc = m_token.source_location;
  expect_token(this, P_TOK_SEMI);
  return m_sema.act_on_return_stmt(ret_expr, { loc_begin, semi_loc + 1 }, semi_loc);
}

// if_stmt:
//     "if" expr compound_stmt
//     "if" expr compound_stmt "else" (compound_stmt | if_stmt)
PAst*
PParser::parse_if_stmt()
{
  assert(lookahead(P_TOK_KEY_if));

  PSourceLocation loc_begin = m_token.source_location;
  consume();

  PAstExpr* cond_expr = parse_expr();
  PAst* then_stmt = parse_compound_stmt();

  PSourceLocation loc_end;
  PAst* else_stmt = nullptr;
  if (lookahead(P_TOK_KEY_else)) {
    consume();

    if (lookahead(P_TOK_KEY_if)) {
      else_stmt = parse_if_stmt();
    } else {
      else_stmt = parse_compound_stmt();
    }

    loc_end = else_stmt->get_source_range().end;
  } else {
    loc_end = then_stmt->get_source_range().end;
  }

  auto* node = m_sema.act_on_if_stmt(cond_expr, then_stmt, else_stmt, { loc_begin, loc_end });
  return node;
}

// loop_stmt:
//     "loop" compound_stmt
PAst*
PParser::parse_loop_stmt()
{
  assert(lookahead(P_TOK_KEY_loop));

  PSourceLocation loc_begin = m_token.source_location;
  consume();

  m_sema.act_before_loop_stmt_body();
  PAst* body = parse_compound_stmt();

  const auto range = PSourceRange{ loc_begin, body->get_source_range().end };
  auto* node = m_sema.act_on_loop_stmt(body, range);
  return node;
}

// while_stmt:
//     "while" expr compound_stmt
PAst*
PParser::parse_while_stmt()
{
  assert(lookahead(P_TOK_KEY_while));

  PSourceLocation loc_begin = m_token.source_location;
  consume();

  PAstExpr* cond_expr = parse_expr();

  m_sema.act_before_while_stmt_body();
  PAst* body = parse_compound_stmt();

  const auto range = PSourceRange{ loc_begin, body->get_source_range().end };
  auto* node = m_sema.act_on_while_stmt(cond_expr, body, range);
  return node;
}

// expr_stmt:
//     expr ";"
PAst*
PParser::parse_expr_stmt()
{
  PAst* expr = parse_expr();
  expect_token(this, P_TOK_SEMI);
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
  consume();
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
  consume();
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
  consume();
  return m_sema.act_on_float_literal(value, suffix, range);
}

// paren_expr:
//     "(" expr ")"
PAstExpr*
PParser::parse_paren_expr()
{
  assert(lookahead(P_TOK_LPAREN));

  PSourceLocation begin_loc = m_token.source_location;
  consume();

  PAstExpr* sub_expr = nullptr;
  if (lookahead(P_TOK_RPAREN) || lookahead(P_TOK_EOF)) {
    PDiag* d = diag_at(P_DK_err_expected_expr, begin_loc + 1);
    diag_add_source_caret(d, begin_loc + 1);
    diag_flush(d);
  } else {
    // FIXME: remove (PAstExpr*) cast
    sub_expr = parse_expr();
  }

  PSourceLocation end_loc = m_token.source_location + m_token.token_length;
  expect_token(this, P_TOK_RPAREN);
  return m_sema.act_on_paren_expr(sub_expr, { begin_loc, end_loc });
}

// decl_ref_expr:
//     IDENTIFIER
PAstExpr*
PParser::parse_decl_ref_expr()
{
  assert(m_token.kind == P_TOK_IDENTIFIER);

  const PSourceRange range = get_token_range();
  PIdentifierInfo* name = m_token.data.identifier;
  consume();

  auto* node = m_sema.act_on_decl_ref_expr(name, range);
  return node;
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
      return parse_decl_ref_expr();
    default:
      unexpected_token(this);
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
  assert(lookahead(P_TOK_LPAREN));

  PSourceLocation lparen_loc = m_token.source_location;
  consume(); // consume '('

  // Parse arguments:
  std::vector<PAstExpr*> args;
  while (!lookahead(P_TOK_RPAREN)) {
    PAstExpr* arg = parse_expr();
    args.push_back(arg);

    if (!lookahead(P_TOK_COMMA))
      break;

    consume(); // consume ','
  }

  PSourceLocation end_loc = m_token.source_location + m_token.token_length;
  expect_token(this, P_TOK_RPAREN);

  const auto range = PSourceRange{ p_callee->get_source_range().begin, end_loc };
  auto* node = m_sema.act_on_call_expr(p_callee, args, range, lparen_loc);
  return node;
}

// member_expr:
//     postfix_expr "." IDENTIFIER
PAstExpr*
PParser::parse_member_expr(PAstExpr* p_base_expr)
{
  assert(lookahead(P_TOK_DOT));

  PSourceLocation dot_loc = m_token.source_location;
  consume(); // consume '.'

  if (!lookahead(P_TOK_IDENTIFIER)) {
    unexpected_token(this);
    return nullptr;
  }

  PSourceRange name_range = get_token_range();
  PIdentifierInfo* name = m_token.data.identifier;
  consume(); // consume identifier

  assert(false && "not yet implemented");
  return nullptr;
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

// postfix_expr:
//     call_expr
//     "-" unary_expr
//     "!" unary_expr
//     "&" unary_expr
//     "*" deref_expr
PAstExpr*
PParser::parse_unary_expr()
{
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

  PSourceLocation op_loc = m_token.source_location;
  consume(); // consume opcode

  PAstExpr* sub_expr = parse_unary_expr();

  const auto range = PSourceRange{ op_loc, sub_expr->get_source_range().end };
  auto* node = m_sema.act_on_unary_expr(sub_expr, opcode, range, op_loc);
  return node;
}

// cast_expr:
//     unary_expr
//     unary_expr "as" type
PAstExpr*
PParser::parse_cast_expr()
{
  PAstExpr* sub_expr = parse_unary_expr();

  if (lookahead(P_TOK_KEY_as)) {
    PSourceLocation as_loc = m_token.source_location;
    consume();

    PType* target_ty = parse_type();
    const auto range = PSourceRange{ sub_expr->get_source_range().begin, as_loc + 2 };
    auto* node = m_sema.act_on_cast_expr(sub_expr, target_ty, range, as_loc);
    return node;
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

PAstExpr*
PParser::parse_binary_expr(PAstExpr* p_lhs, int p_expr_prec)
{
  while (true) {
    int tok_prec = get_binop_precedence(m_token.kind);

    /* If this is a binop that binds at least as tightly as the current
     * binary_op, consume it, otherwise we are done. */
    if (tok_prec < p_expr_prec)
      return p_lhs;

    /* Okay, we know this is a binary_op. */
    const PAstBinaryOp binary_opcode = get_binop_from_tok_kind(m_token.kind);
    const PSourceLocation op_loc = m_token.source_location;
    consume();

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
//     "fn" identifier "(" param_decl_list? ")" func_type_specifier? ";"
//     "fn" identifier "(" param_decl_list? ")" func_type_specifier? compound_stmt
//
// func_type_specifier:
//     "->" type
static PDecl*
parse_func_decl(PParser* p_parser)
{
  assert(p_parser->lookahead(P_TOK_KEY_fn));

  PSourceLocation key_fn_end_loc = LOOKAHEAD_END_LOC;
  consume_token(p_parser);

  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = nullptr;
  if (p_parser->lookahead(P_TOK_IDENTIFIER)) {
    name = p_parser->m_token.data.identifier;
    consume_token(p_parser); // consume identifier
  }

  std::vector<PParamDecl*> params;

  // Parse parameters
  expect_token(p_parser, P_TOK_LPAREN);

  p_parser->m_sema.push_scope(P_SF_FUNC_PARAMS);
  if (!p_parser->lookahead(P_TOK_RPAREN))
    params = p_parser->parse_param_list();
  p_parser->m_sema.pop_scope();

  PSourceLocation rparen_loc = LOOKAHEAD_BEGIN_LOC;
  if (p_parser->lookahead(P_TOK_EOF)) {
    print_expect_tok_diag(p_parser, P_TOK_RPAREN, p_parser->prev_lookahead_end_loc);
    return nullptr;
  }

  expect_token(p_parser, P_TOK_RPAREN);

  // Parse return type specified (optional)
  PType* ret_ty = nullptr;
  if (p_parser->lookahead(P_TOK_ARROW)) {
    consume_token(p_parser); // consume '->'
    ret_ty = p_parser->parse_type();
  }

  PFunctionDecl* decl = p_parser->m_sema.act_on_func_decl(name, ret_ty, params, name_range, key_fn_end_loc);

  // Parse body
  if (!p_parser->lookahead(P_TOK_LBRACE)) {
    PDiag* d = diag_at(P_DK_err_expected_func_body_after_func_decl, rparen_loc + 1);
    diag_add_source_caret(d, rparen_loc + 1);
    diag_flush(d);
    return decl;
  }

  p_parser->m_sema.begin_func_decl_analysis(decl);
  decl->body = p_parser->parse_compound_stmt();
  p_parser->m_sema.end_func_decl_analysis();

  return decl;
}

// struct_field_decl:
//     IDENTIFIER type_specifier ";"
static PDecl*
parse_struct_field_decl(PParser* p_parser)
{
  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = nullptr;
  if (p_parser->lookahead(P_TOK_IDENTIFIER))
    name = p_parser->m_token.data.identifier;

  if (!expect_token(p_parser, P_TOK_IDENTIFIER))
    return nullptr;

  expect_token(p_parser, P_TOK_COLON);
  PType* type = p_parser->parse_type();

  expect_token(p_parser, P_TOK_SEMI);

  assert(false && "not yet implemented");
  return nullptr;
}

// struct_field_decls:
//     struct_field_decl
//     struct_field_decls struct_field_decl
static void
parse_struct_field_decls(PParser* p_parser, std::vector<PDecl*>& p_fields)
{
  while (!p_parser->lookahead(P_TOK_RBRACE)) {
    PDecl* field = parse_struct_field_decl(p_parser);
    if (field != nullptr)
      p_fields.push_back(field);
  }
}

// struct_decl:
//     "struct" identifier "{" "}"
static PDecl*
parse_struct_decl(PParser* p_parser)
{
  assert(p_parser->lookahead(P_TOK_KEY_struct));

  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  consume_token(p_parser); // consume 'struct'

  PSourceRange name_range = { LOOKAHEAD_BEGIN_LOC, LOOKAHEAD_END_LOC };
  PIdentifierInfo* name = nullptr;
  if (p_parser->lookahead(P_TOK_IDENTIFIER)) {
    name = p_parser->m_token.data.identifier;
    consume_token(p_parser); // consume identifier
  } else {
    // Act as if there was an identifier but emit a diagnostic.
    print_expect_tok_diag(p_parser, P_TOK_IDENTIFIER, p_parser->prev_lookahead_end_loc);
  }

  std::vector<PDecl*> fields;

  expect_token(p_parser, P_TOK_LBRACE);
  parse_struct_field_decls(p_parser, fields);
  expect_token(p_parser, P_TOK_RBRACE);

  assert(false && "not yet implemented");
  return nullptr;
}

// top_level_decl:
//     function_decl
//     struct_decl
static PDecl*
parse_top_level_decl(PParser* p_parser)
{
  switch (p_parser->m_token.kind) {
    case P_TOK_KEY_fn:
      return parse_func_decl(p_parser);
    case P_TOK_KEY_struct:
      return parse_struct_decl(p_parser);
    default:
      unexpected_token(p_parser);
      return nullptr;
  }
}

// top_level_decls:
//     top_level_decl
//     top_level_decls top_level_decl
static void
parse_top_level_decls(PParser* p_parser, std::vector<PDecl*>& p_decls)
{
  while (!p_parser->lookahead(P_TOK_EOF)) {
    PDecl* decl = parse_top_level_decl(p_parser);
    if (decl != nullptr)
      p_decls.push_back(decl);
  }
}

// translation_unit:
//     function_decls
//
// function_decls:
//     function_decl
//     function_decls function_decl
static PAst*
parse_translation_unit(PParser* p_parser)
{
  PSourceLocation loc_begin = LOOKAHEAD_BEGIN_LOC;
  p_parser->m_sema.push_scope(P_SF_NONE);

  std::vector<PDecl*> decls;
  parse_top_level_decls(p_parser, decls);

  expect_token(p_parser, P_TOK_EOF);

  auto** raw_decls = p_parser->context.alloc_object<PDecl*>(decls.size());
  std::copy(decls.begin(), decls.end(), raw_decls);

  auto* node = p_parser->context.new_object<PAstTranslationUnit>(
    raw_decls, decls.size(), PSourceRange{ loc_begin, LOOKAHEAD_BEGIN_LOC });
  p_parser->m_sema.pop_scope();
  return node;
}

PAst*
p_parse(PParser* p_parser)
{
  assert(p_parser != nullptr);

  // We do not call consume_token() because it sets p_parser->prev_lookahead_end_loc
  // to the end position of m_token which is undefined here.
  p_parser->prev_lookahead_end_loc = 0;
  lexer_next(&p_parser->lexer, &p_parser->m_token);

  PAst* ast = parse_translation_unit(p_parser);
  return ast;
}

void
PParser::consume()
{
  prev_lookahead_end_loc = m_token.source_location + m_token.token_length;
  lexer_next(&lexer, &m_token);
}
