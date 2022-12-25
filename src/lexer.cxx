#include "lexer.hxx"

#include "utils/diag.hxx"

#include <cassert>

void
lexer_init(PLexer* p_lexer)
{
  assert(p_lexer != nullptr);

  p_lexer->keep_comments = false;

  p_lexer->source_file = nullptr;
  p_lexer->cursor = nullptr;
}

void
lexer_set_source_file(PLexer* p_lexer, PSourceFile* p_source_file)
{
  assert(p_lexer != nullptr && p_source_file != nullptr);

  g_current_source_file = p_source_file;
  g_current_source_location = 0;

  p_lexer->source_file = p_source_file;
  p_lexer->cursor = p_lexer->source_file->get_buffer_raw();
}

void
lexer_destroy(PLexer* p_lexer)
{
  assert(p_lexer != nullptr);
}
