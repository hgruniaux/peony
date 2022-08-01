#include "token_kind.h"

#include <stddef.h>

const char*
p_token_kind_get_name(PTokenKind p_token_kind)
{
  switch (p_token_kind) {
#define TOKEN(kind)                                                            \
  case P_TOK_##kind:                                                           \
    return #kind;
#define KEYWORD(spelling)                                                      \
  case P_TOK_KEY_##spelling:                                                   \
    return "KEY_" #spelling;
#include "token_kinds.def"
    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }
}

const char*
p_token_kind_get_spelling(PTokenKind p_token_kind)
{
  switch (p_token_kind) {
#define TOKEN(kind)                                                            \
  case P_TOK_##kind:                                                           \
    return NULL;
#define PUNCTUACTOR(kind, spelling)                                            \
  case P_TOK_##kind:                                                           \
    return spelling;
#define KEYWORD(spelling)                                                      \
  case P_TOK_KEY_##spelling:                                                   \
    return #spelling;
#include "token_kinds.def"
    default:
      HEDLEY_UNREACHABLE_RETURN(NULL);
  }
}

bool
p_token_kind_is_keyword(PTokenKind p_token_kind)
{
  switch (p_token_kind) {
#define TOKEN(kind)
#define KEYWORD(spelling) case P_TOK_KEY_##spelling:
#include "token_kinds.def"
    return true;
    default:
      return false;
  }
}
