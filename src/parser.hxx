#ifndef PEONY_PARSER_HXX
#define PEONY_PARSER_HXX

#include "ast/ast.hxx"
#include "context.hxx"
#include "lexer.hxx"
#include "sema.hxx"

class PParser
{
public:
  PContext& context;
  // The lexer to use by the parser to get the tokens.
  PLexer& lexer;
  // The token actually considered by the parser.
  PToken m_token;
  // The source location at end of the previous lookahead. At the first token,
  // this is set to the start of file.
  PSourceLocation prev_lookahead_end_loc;
  PSema m_sema;

  PParser(PContext& p_context, PLexer& p_lexer);
  ~PParser();

  [[nodiscard]] bool lookahead(PTokenKind p_kind) const { return m_token.kind == p_kind; }

  PType* parse_type();
  PAst* parse_stmt();
  PAstExpr* parse_expr();

  PParamDecl* parse_param_decl();
  std::vector<PParamDecl*> parse_param_list();
  PVarDecl* parse_var_decl();
  std::vector<PVarDecl*> parse_var_list();

  PAst* parse_compound_stmt();
  PAst* parse_let_stmt();
  PAst* parse_break_stmt();
  PAst* parse_continue_stmt();
  PAst* parse_return_stmt();
  PAst* parse_if_stmt();
  PAst* parse_loop_stmt();
  PAst* parse_while_stmt();
  PAst* parse_expr_stmt();

  PAstExpr* parse_bool_lit();
  PAstExpr* parse_int_lit();
  PAstExpr* parse_float_lit();
  PAstExpr* parse_paren_expr();
  PAstExpr* parse_decl_ref_expr();
  PAstExpr* parse_primary_expr();
  PAstExpr* parse_call_expr(PAstExpr* p_callee);
  PAstExpr* parse_member_expr(PAstExpr* p_base_expr);
  PAstExpr* parse_postfix_expr();
  PAstExpr* parse_unary_expr();
  PAstExpr* parse_cast_expr();
  PAstExpr* parse_binary_expr(PAstExpr* p_lhs, int p_expr_prec);

private:
  [[nodiscard]] PSourceRange get_token_range() const
  {
    return { m_token.source_location, m_token.source_location + m_token.token_length };
  }

  void consume();
};

PAst*
p_parse(struct PParser* p_parser);

#endif // PEONY_PARSER_HXX
