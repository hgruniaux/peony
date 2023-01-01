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

void
PLexer::fill_token(PToken& p_token, PTokenKind p_kind)
{
  p_token.kind = p_kind;
  p_token.source_location = m_marked_source_location;
  p_token.token_length = m_cursor - m_marked_cursor;
}

void
PLexer::register_newline()
{
  uint32_t line_pos = m_cursor - source_file->get_buffer_raw();
  source_file->get_line_map().add(line_pos);
}

void
PLexer::fill_identifier_token(PToken& p_token, bool p_is_raw)
{
  const char* spelling_begin = p_is_raw ? m_marked_cursor + 2 : m_marked_cursor;
  PIdentifierInfo* ident = p_identifier_table_get(identifier_table, spelling_begin, m_cursor);
  assert(ident != nullptr);
  fill_token(p_token, p_is_raw ? P_TOK_IDENTIFIER : ident->token_kind);
  p_token.data.identifier = ident;
}

static void
parse_int_suffix(PToken& p_token)
{
  PIntLiteralSuffix suffix = P_ILS_NO_SUFFIX;

  if (p_token.data.literal.end - p_token.data.literal.begin >= 2) {
    const char* suffix_start = p_token.data.literal.end - 2;
    if (*suffix_start == 'i' || *suffix_start == 'u') {
      bool is_unsigned = *suffix_start == 'u';
      ++suffix_start;
      assert(*suffix_start == '8');
      p_token.data.literal.end -= 2;
      if (is_unsigned)
        suffix = P_ILS_U8;
      else
        suffix = P_ILS_I8;
    }
  }

  if (p_token.data.literal.end - p_token.data.literal.begin >= 3) {
    const char* suffix_start = p_token.data.literal.end - 3;
    if (*suffix_start == 'i' || *suffix_start == 'u') {
      bool is_unsigned = *suffix_start == 'u';
      ++suffix_start;
      assert(*suffix_start == '1' || *suffix_start == '3' || *suffix_start == '6');
      ++suffix_start;

      if (*suffix_start == '2') {
        if (is_unsigned)
          suffix = P_ILS_U32;
        else
          suffix = P_ILS_I32;
      } else if (*suffix_start == '4') {
        if (is_unsigned)
          suffix = P_ILS_U64;
        else
          suffix = P_ILS_I64;
      } else { // *suffix_start == '6'
        if (is_unsigned)
          suffix = P_ILS_U16;
        else
          suffix = P_ILS_I16;
      }

      p_token.data.literal.end -= 3;
    }
  }

  p_token.data.literal.suffix_kind = suffix;
}

void
PLexer::fill_integer_literal(PToken& p_token, int p_radix)
{
  fill_token(p_token, P_TOK_INT_LITERAL);
  p_token.data.literal.int_radix = p_radix;
  p_token.data.literal.begin = m_marked_cursor;
  p_token.data.literal.end = m_cursor; // suffix is removed in parse_int_suffix().
  parse_int_suffix(p_token);
}

void
PLexer::fill_string_literal(PToken& p_token)
{
  fill_token(p_token, P_TOK_STRING_LITERAL);
  p_token.data.literal.begin = m_marked_cursor;
  p_token.data.literal.end = m_cursor;
}
