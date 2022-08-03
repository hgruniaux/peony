#pragma once

#include "ast.h"
#include "lexer.h"
#include "sema.h"

struct PParser
{
  // The lexer to use by the parser to get the tokens.
  PLexer* lexer;
  // If not NULL then the tokens will be taken from it until there is no more
  // tokens in the run. Then this will be set to NULL.
  // This is used for example to parse lazily function bodies.
  PDynamicArray* token_run;
  // The current index in token_run if we are reading tokens from there.
  // This is incremented by consume_token().
  int current_idx_in_token_run;

  // The token actually considered by the parser.
  PToken lookahead;
  // The source location at end of the previous lookahead. At the first token,
  // this is set to the start of file.
  PSourceLocation prev_lookahead_end_loc;

  PSema sema;
};

void
p_parser_init(struct PParser* p_parser);

void
p_parser_destroy(struct PParser* p_parser);

PAst*
p_parse(struct PParser* p_parser);
