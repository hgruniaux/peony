#pragma once

#include <hedley.h>
#include <stdbool.h>

HEDLEY_BEGIN_C_DECLS

// All the supported token kinds.
typedef enum PTokenKind
{
#define TOKEN(kind) P_TOK_##kind,
#include "token_kinds.def"
} PTokenKind;

// Returns a debug suitable string representation of the given token kind.
const char*
token_kind_get_name(PTokenKind p_token_kind);

// Gets the spelling of the given token kind if it can be determined.
// For tokens that are specials or do not have a dynamic spelling (e.g.
// literals, identifiers, EOF, etc.) this will returns NULL.
const char*
token_kind_get_spelling(PTokenKind p_token_kind);

// Checks if the token kind corresponds to a keyword (P_TOK_KEY_*).
bool
token_kind_is_keyword(PTokenKind p_token_kind);

// Checks if the token kind corresponds to a punctuactor or operator
// (e.g. `+` or `(`).
bool
token_kind_is_punctuactor(PTokenKind p_token_kind);

HEDLEY_END_C_DECLS
