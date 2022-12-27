#include "lexer.hxx"

#include "utils/diag.hxx"

#include <cassert>

PLexer::PLexer()
{
  source_file = nullptr;
}

void
PLexer::set_source_file(PSourceFile* p_source_file)
{
  assert(p_source_file != nullptr);

  g_current_source_file = p_source_file;

  source_file = p_source_file;
  m_cursor = source_file->get_buffer_raw();
}
