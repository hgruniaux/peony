#pragma once

#include "ast.h"
#include "lexer.h"
#include "sema.h"

struct PParser
{
  PLexer* lexer;
  PToken lookahead;

  PSema sema;
};

void
p_parser_init(struct PParser* p_parser);

void
p_parser_destroy(struct PParser* p_parser);

PAst*
p_parse(struct PParser* p_parser);
