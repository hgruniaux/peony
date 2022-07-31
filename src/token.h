#pragma once

#include "token_kind.h"

struct PIdentifierInfo;

typedef struct PToken
{
  PTokenKind kind;
  union
  {
    struct PIdentifierInfo* identifier_info;
    struct
    {
      const char* literal_begin;
      const char* literal_end;
    };
  } data;
} PToken;

const char*
p_get_token_spelling(struct PToken* p_token);

/* Dumps token info to stdout for debugging purposes. */
void
p_token_dump(struct PToken* p_token);
