#pragma once

#include "token_kind.h"
#include "utils/source_location.h"

struct PIdentifierInfo;

typedef struct PToken
{
  PTokenKind kind;
  PSourceLocation source_location;
  uint32_t token_length;
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
p_get_token_spelling(PToken* p_token);

/* Dumps token info to stdout for debugging purposes. */
void
p_token_dump(PToken* p_token);
