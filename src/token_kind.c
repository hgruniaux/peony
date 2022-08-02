#include "token_kind.h"

#include <stddef.h>

const char*
token_kind_get_name(PTokenKind p_token_kind)
{
  switch (p_token_kind) {
#define TOKEN(kind)                                                                                                    \
  case P_TOK_##kind:                                                                                                   \
    return #kind;
#define KEYWORD(spelling)                                                                                              \
  case P_TOK_KEY_##spelling:                                                                                           \
    return "KEY_" #spelling;
#include "token_kinds.def"
    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }
}

const char*
token_kind_get_spelling(PTokenKind p_token_kind)
{
  switch (p_token_kind) {
#define TOKEN(p_kind)
#define PUNCTUACTOR(p_kind, p_spelling)                                                                                \
  case P_TOK_##p_kind:                                                                                                 \
    return p_spelling;
#define KEYWORD(p_spelling)                                                                                            \
  case P_TOK_KEY_##p_spelling:                                                                                         \
    return #p_spelling;
#include "token_kinds.def"
    default:
      return NULL;
  }
}

bool
token_kind_is_keyword(PTokenKind p_token_kind)
{
  switch (p_token_kind) {
#define TOKEN(p_kind)
#define KEYWORD(p_spelling) case P_TOK_KEY_##p_spelling:
#include "token_kinds.def"
    return true;
    default:
      return false;
  }
}

bool
token_kind_is_punctuactor(PTokenKind p_token_kind)
{
  switch (p_token_kind) {
#define TOKEN(p_kind)
#define PUNCTUACTOR(p_kind, p_spelling)                                                                                \
  case P_TOK_##p_kind:                                                                                                 \
    return true;
#include "token_kinds.def"
    default:
      return false;
  }
}
