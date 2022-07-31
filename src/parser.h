#pragma once

#include "ast.h"
#include "lexer.h"
#include "sema.h"

struct PParser
{
  struct PLexer* lexer;
  PToken lookahead;

  PSema sema;

  /*
   * This is incremented each time we enter a loop body, and
   * likewise, decremented each time we leave it.
   * This is used to detect if we are in a context were continue
   * or break statements are allowed.
   */
  unsigned int loop_depth;
};

void
p_parser_init(struct PParser* p_parser);

void
p_parser_destroy(struct PParser* p_parser);

struct PAst*
p_parse(struct PParser* p_parser);
