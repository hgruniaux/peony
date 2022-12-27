#include "token_kind.hxx"

#include <hedley.h>

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
#include "token_kind.def"
    default:
      HEDLEY_UNREACHABLE_RETURN(nullptr);
  }
}

const char*
token_kind_get_spelling(PTokenKind p_token_kind)
{
  switch (p_token_kind) {
#define TOKEN(p_kind)
#define PUNCTUATION(p_kind, p_spelling)                                                                                \
  case P_TOK_##p_kind:                                                                                                 \
    return p_spelling;
#define KEYWORD(p_spelling)                                                                                            \
  case P_TOK_KEY_##p_spelling:                                                                                         \
    return #p_spelling;
#include "token_kind.def"
    default:
      return nullptr;
  }
}
