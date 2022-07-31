#include "lexer.h"

#include "utils/diag.h"

#include <assert.h>

void
p_lexer_init(struct PLexer* p_lexer)
{
  assert(p_lexer != NULL);

  p_identifier_table_register_keywords(p_lexer->identifier_table);

  p_lexer->b_keep_comments = 0;

  p_lexer->filename = "<unknown>";
  p_lexer->lineno = 1;
  CURRENT_FILENAME = p_lexer->filename;
  CURRENT_LINENO = p_lexer->lineno;

  p_lexer->cursor = p_lexer->input;
}

void
p_lexer_destroy(struct PLexer* p_lexer)
{
  assert(p_lexer != NULL);
}
