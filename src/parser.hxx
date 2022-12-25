#pragma once

#include "ast.hxx"
#include "context.hxx"
#include "lexer.hxx"
#include "sema.hxx"

struct PParser
{
  PContext& context;

  // The lexer to use by the parser to get the tokens.
  PLexer& lexer;
  // If not nullptr then the tokens will be taken from it until there is no more
  // tokens in the run. Then this will be set to nullptr.
  // This is used for example to parse lazily function bodies.
  std::vector<PToken>* token_run;
  // The current index in token_run if we are reading tokens from there.
  // This is incremented by consume_token().
  int current_idx_in_token_run;

  // The token actually considered by the parser.
  PToken lookahead;
  // The source location at end of the previous lookahead. At the first token,
  // this is set to the start of file.
  PSourceLocation prev_lookahead_end_loc;

  PSema sema;

  PParser(PContext& p_context, PLexer& p_lexer);
  ~PParser();
};

PAst*
p_parse(struct PParser* p_parser);
