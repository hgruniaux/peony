#include "lexer.h"

#include "utils/diag.h"

#include <assert.h>

void
p_lexer_init(PLexer* p_lexer, PSourceFile* p_source_file)
{
  assert(p_lexer != NULL && p_source_file != NULL);

  p_identifier_table_register_keywords(p_lexer->identifier_table);

  CURRENT_FILENAME = p_source_file->filename;
  CURRENT_LINENO = 1;

  p_lexer->source_file = p_source_file;
  p_lexer->cursor = p_lexer->source_file->buffer;
}

void
p_lexer_destroy(PLexer* p_lexer)
{
  assert(p_lexer != NULL);
}
