#ifndef PEONY_TOKEN_KIND_HXX
#define PEONY_TOKEN_KIND_HXX

/// All the supported token kinds.
enum PTokenKind
{
#define TOKEN(kind) P_TOK_##kind,
#include "token_kind.def"
};

/// Returns a debug suitable string representation of the given token m_kind.
const char*
token_kind_get_name(PTokenKind p_token_kind);

/// Gets the spelling of the given token m_kind if it can be determined.
/// For tokens that are specials or do not have a dynamic spelling (e.g.
/// literals, identifiers, EOF, etc.) this will returns nullptr.
const char*
token_kind_get_spelling(PTokenKind p_token_kind);

#endif // PEONY_TOKEN_KIND_HXX
