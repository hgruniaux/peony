#pragma once

#include "identifier_table.h"
#include "token.h"
#include "utils/dynamic_array.h"
#include "utils/source_file.h"

// The lexical analyzer interface.
typedef struct PLexer
{
  PIdentifierTable* identifier_table;
  PSourceFile* source_file;

  // For re2c:
  const char* cursor;
  const char* marker;
  const char* marked_cursor;
  PSourceLocation marked_source_location;
} PLexer;

void
lexer_init(PLexer* p_lexer, PSourceFile* p_source_file);

void
lexer_destroy(PLexer* p_lexer);

// Gets the next token from the lexer and stores it in p_token.
// If end of file is reached, then p_token is of kind P_TOK_EOF
// and all following calls will also returns P_TOK_EOF.
// All unknown characters are ignored however a diagnosis will
// still be issued.
void
lexer_next(PLexer* p_lexer, PToken* p_token);
