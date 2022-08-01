#pragma once

#include "utils/hedley.h"

#include <stdbool.h>

typedef enum PTokenKind
{
#define TOKEN(kind) P_TOK_##kind,
#include "token_kinds.def"
} PTokenKind;

/* Returns the token name suitable for debugging display. */
const char*
p_token_kind_get_name(PTokenKind p_token_kind);

/* Returns the canonical spelling of the requested token or NULL
 * if there is no canonical spelling (e.g. TOK_IDENTIFIER). */
const char*
p_token_kind_get_spelling(PTokenKind p_token_kind);

/* Returns true if p_token_kind corresponds to a keyword. */
bool
p_token_kind_is_keyword(PTokenKind p_token_kind);
