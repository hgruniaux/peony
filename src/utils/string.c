#include "string.h"

#include <string.h>

void
string_append(PString* p_string, const char* p_other)
{
  size_t other_len = strlen(p_other);
  dyn_array_reserve_impl(p_string, sizeof(char), p_string->size + other_len + 1);
  memcpy(p_string->buffer + p_string->size, p_other, other_len + 1 /* include NUL-terminator */);
  p_string->size += other_len;
  p_string->buffer[p_string->size] = '\0';
}

void
string_push_back(PString* p_string, char p_char)
{
  dyn_array_reserve_impl(p_string, sizeof(char), p_string->size + 2);
  p_string->buffer[p_string->size++] = p_char;
  p_string->buffer[p_string->size] = '\0';
}
