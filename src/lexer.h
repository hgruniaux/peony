#pragma once

#include "identifier_table.h"
#include "token.h"

struct PLexer
{
  struct PIdentifierTable* identifier_table;
  const char* input;

  unsigned int b_keep_comments;

  /* Rudimentary location tracking: */
  const char* filename;
  unsigned int lineno;

  /* For re2c: */
  const char* cursor;
};

void
p_lexer_init(struct PLexer* p_lexer);

void
p_lexer_destroy(struct PLexer* p_lexer);

void
p_lex(struct PLexer* p_lexer, struct PToken* p_token);
