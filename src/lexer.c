#include "lexer.h"

#include "utils/diag.h"

#include <assert.h>

void
p_lexer_init(struct PLexer* p_lexer)
{
  assert(p_lexer != NULL);

  p_identifier_table_register_keywords(p_lexer->identifier_table);

  p_lexer->b_keep_comments = g_verify_mode_enabled;

  CURRENT_FILENAME = "<unknown>";
  CURRENT_LINENO = 1;

  p_lexer->cursor = p_lexer->input;
}

void
p_lexer_destroy(struct PLexer* p_lexer)
{
  assert(p_lexer != NULL);
}
