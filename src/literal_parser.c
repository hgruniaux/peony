#include "literal_parser.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

/* Do `*p_acc += p_value` checking for overflow (returns true if overflow). */
static bool
safe_add(uintmax_t* p_acc, uintmax_t p_value)
{
#ifdef __GNUC__
  return __builtin_add_overflow(*p_acc, p_value, p_acc);
#else
  if (*p_acc > (UINTMAX_MAX - p_value)) {
    // overflow
    return true;
  }

  *p_acc += p_value;
  return false;
#endif
}

/* Do `*p_acc *= p_value` checking for overflow (returns true if overflow). */
static bool
safe_mul(uintmax_t* p_acc, uintmax_t p_value)
{
#ifdef __GNUC__
  return __builtin_mul_overflow(*p_acc, p_value, p_acc);
#else
  if (p_value != 0 && *p_acc > INT_MAX / p_value) {
    // overflow
    return true;
  }

  *p_acc *= p_value;
  return false;
#endif
}

bool
parse_dec_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value)
{
  assert(p_begin != NULL && p_begin != p_end && p_value != NULL);

  *p_value = 0;
  while (*p_begin != *p_end) {
    switch (*p_begin) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        bool overflow = safe_mul(p_value, 10);
        overflow |= safe_add(p_value, *p_begin++ - '0');
        if (overflow)
          return true;
      }
      break;
      case '_': // digit separator
        p_begin++;
        continue;
      default:
        assert(false && "invalid character in integer literal");
    }
  }

  return false;
}

bool
parse_bin_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value)
{
  assert(p_begin != NULL && p_begin != p_end && p_value != NULL);
  assert(p_begin[0] == '0' && (p_begin[1] == 'b' || p_begin[1] == 'B'));

  p_begin += 2; // skip '0b'

  *p_value = 0;
  while (*p_begin != *p_end) {
    switch (*p_begin) {
      case '0':
      case '1': {
        bool overflow = safe_mul(p_value, 2);
        overflow |= safe_add(p_value, *p_begin++ - '0');
        if (overflow)
          return true;
      }
      break;
      case '_': // digit separator
        p_begin++;
        continue;
      default:
        assert(false && "invalid character in integer literal");
    }
  }

  return false;
}

bool
parse_hex_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value)
{
  assert(p_begin != NULL && p_begin != p_end && p_value != NULL);
  assert(p_begin[0] == '0' && (p_begin[1] == 'x' || p_begin[1] == 'X'));

  p_begin += 2; // skip '0x'

  *p_value = 0;
  while (*p_begin != *p_end) {
    switch (*p_begin) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        bool overflow = safe_mul(p_value, 16);
        overflow |= safe_add(p_value, *p_begin++ - '0');
        if (overflow)
          return true;
      }
      break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f': {
        bool overflow = safe_mul(p_value, 16);
        overflow |= safe_add(p_value, (uintmax_t)(*p_begin++ - 'a') + 10);
        if (overflow)
          return true;
      }
      break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F': {
        bool overflow = safe_mul(p_value, 16);
        overflow |= safe_add(p_value, (uintmax_t)(*p_begin++ - 'A') + 10);
        if (overflow)
          return true;
      }
      break;
      case '_': // digit separator
        p_begin++;
        continue;
      default:
        assert(false && "invalid character in integer literal");
    }
  }

  return false;
}

bool
parse_float_literal_token(const char* p_begin, const char* p_end, double* p_value)
{
  assert(p_begin != NULL && p_begin != p_end && p_value != NULL);

  const size_t buffer_size = sizeof(char) * (p_end - p_begin + 1 /* NUL-terminated */);
  char* buffer = malloc(buffer_size);
  assert(buffer != NULL);

  char* it = buffer;
  while (p_begin != p_end) {
    if (*p_begin == '_') {
      // digit separator
      ++p_begin;
      continue;
    }

    *it++ = *p_begin++;
  }
  *it = '\0';

  // This expects that the current locale is the "C" locale (this is set by the driver).
  errno = 0;
  *p_value = strtod(buffer, NULL);
  bool too_big = (errno == ERANGE);

  free(buffer);
  return too_big;
}
