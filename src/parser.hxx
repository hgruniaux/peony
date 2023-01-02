#ifndef PEONY_PARSER_HXX
#define PEONY_PARSER_HXX

#include "ast/ast.hxx"
#include "context.hxx"
#include "lexer.hxx"
#include "sema.hxx"

class PParser
{
public:
  PParser(PContext& p_context, PLexer& p_lexer);
  ~PParser();

  [[nodiscard]] bool lookahead(PTokenKind p_kind) const { return m_token.kind == p_kind; }

  PAstTranslationUnit* parse();
  PAst* parse_standalone_stmt();
  PAstExpr* parse_standalone_expr();

private:
  PAst* parse_stmt();
  PAstExpr* parse_expr();
  PType* parse_type();

  PParamDecl* parse_param_decl();
  std::vector<PParamDecl*> parse_param_list();
  PVarDecl* parse_var_decl();
  std::vector<PVarDecl*> parse_var_list();
  PFunctionDecl* parse_func_decl(bool p_is_extern = false);
  PDecl* parse_extern_decl();
  PStructDecl* parse_struct_decl();
  PStructFieldDecl* parse_struct_field_decl();

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
  PAstStructFieldExpr* parse_struct_field_expr(PStructDecl* p_struct_decl);
  PAstExpr* parse_struct_expr(PLocalizedIdentifierInfo p_name);
  PAstExpr* parse_primary_expr();
  PAstExpr* parse_call_expr(PAstExpr* p_callee);
  PAstExpr* parse_member_expr(PAstExpr* p_base_expr);
  PAstExpr* parse_postfix_expr();
  PAstExpr* parse_unary_expr();
  PAstExpr* parse_cast_expr();
  PAstExpr* parse_binary_expr(PAstExpr* p_lhs, int p_expr_prec);

  PDecl* parse_top_level_decl();
  PAstTranslationUnit* parse_translation_unit();

  PType* try_parse_type_specifier();

private:
  friend class PBalancedDelimiterTracker;
  friend class PSourceRangeTracker;

  template<class Fn>
  PLocalizedIdentifierInfo parse_identifier(Fn p_err_fn)
  {
    if (lookahead(P_TOK_IDENTIFIER)) {
      auto info = PLocalizedIdentifierInfo{
        m_token.data.identifier,
        get_token_range()
      };

      consume_token();
      return info;
    } else {
      p_err_fn();
      return PLocalizedIdentifierInfo{
        nullptr,
        {}
      };
    }
  }

  [[nodiscard]] PSourceRange get_token_range() const
  {
    return { m_token.source_location, m_token.source_location + m_token.token_length };
  }

  void consume_token();
  bool try_consume_token(PTokenKind p_kind);

  bool expect_token(PTokenKind p_kind);
  void unexpected_token();

  void print_expect_tok_diag(PTokenKind p_kind, PSourceLocation p_loc);

private:
  PContext& m_context;
  PLexer& m_lexer;
  PSema m_sema;

  // The token actually considered by the parser.
  PToken m_token;
  // The source location at end of the previous lookahead. At the first token,
  // this is set to the start of file.
  PSourceLocation m_prev_lookahead_end_loc;
};

#endif // PEONY_PARSER_HXX
