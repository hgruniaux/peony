#pragma once

#include "dynamic_array.h"

HEDLEY_BEGIN_C_DECLS

// PString is a wrapper around PDynamicArray that simplify its use as a
// dynamic string type. The string buffer is always guaranteed to be
// NUL-terminated.
typedef PDynamicArray PString;

static void
string_init(PString* p_string)
{
  dyn_array_init_impl(p_string, sizeof(char));
  *p_string->buffer = '\0';
}

static void
string_destroy(PString* p_string)
{
  dyn_array_destroy_impl(p_string);
}

void
string_append(PString* p_string, const char* p_other);

void
string_push_back(PString* p_string, char p_char);

HEDLEY_END_C_DECLS
