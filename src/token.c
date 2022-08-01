#include "token.h"
#include "identifier_table.h"

#include <assert.h>
#include <stdio.h>

const char*
p_get_token_spelling(PToken* p_token)
{
  if (p_token->kind == P_TOK_IDENTIFIER) {
    return p_token->data.identifier_info->spelling;
  } else {
    return p_token_kind_get_spelling(p_token->kind);
  }
}

void
p_token_dump(PToken* p_token)
{
  assert(p_token != NULL);

  printf("%s", p_token_kind_get_name(p_token->kind));

  const char* spelling = p_get_token_spelling(p_token);
  if (spelling != NULL) {
    printf(" ('%s')", spelling);
  }

  printf("\n");
}
