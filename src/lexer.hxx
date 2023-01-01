#ifndef PEONY_LEXER_HXX
#define PEONY_LEXER_HXX

#include "identifier_table.hxx"
#include "token.hxx"
#include "utils/source_file.hxx"

/// The lexical analyzer interface.
class PLexer
{
public:
  PLexer();

  PIdentifierTable* identifier_table;
  PSourceFile* source_file;

  void set_source_file(PSourceFile* p_source_file);

  /// Gets the next token from the lexer and stores it in p_token.
  /// If end of file is reached, then p_token is of m_kind P_TOK_EOF
  /// and all following calls will also return P_TOK_EOF.
  /// All unknown characters are ignored however a diagnosis will
  /// still be issued.
  void tokenize(PToken& p_token);

  void set_keep_comments(bool p_keep) { m_keep_comments = p_keep; }

private:
  void fill_token(PToken& p_token, PTokenKind p_kind);
  void register_newline();

  void fill_identifier_token(PToken& p_token, bool p_is_raw);
  void fill_integer_literal(PToken& p_token, int p_radix);
  void fill_string_literal(PToken& p_token);

private:
  bool m_keep_comments = false;

  // For re2c:
  const char* m_cursor = nullptr;
  const char* m_marker = nullptr;
  const char* m_marked_cursor = nullptr;
  PSourceLocation m_marked_source_location;
};

#endif // PEONY_LEXER_HXX
