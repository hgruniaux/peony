#pragma once

#include "identifier_table.h"
#include "token.h"
#include "utils/source_file.h"

typedef struct PLexer
{
  struct PIdentifierTable* identifier_table;
  PSourceFile* source_file;

  /* For re2c: */
  const char* cursor;
  const char* marked_cursor;
  PSourceLocation marked_source_location;
} PLexer;

void
p_lexer_init(PLexer* p_lexer, PSourceFile* p_source_file);

void
p_lexer_destroy(PLexer* p_lexer);

void
p_lex(PLexer* p_lexer, PToken* p_token);
