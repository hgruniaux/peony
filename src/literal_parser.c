#include "literal_parser.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

/*
 * Do `*p_acc += p_value` checking for overflow. Returns true in case of
 * overflow, otherwise false is returned.
 */
static inline bool
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

/*
 * Do `*p_acc *= p_value` checking for overflow. Returns true in case of
 * overflow, otherwise false is returned.
 */
static inline bool
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

/*
 * Converts `p_ch` to a digit in the given radix (1 to 36).
 */
static inline uintmax_t
to_digit(char p_ch, int p_radix)
{
  // Based on Rust char::to_digit() implementation.
  // If not a digit, a number greater than radix will be created.
  uintmax_t digit = p_ch - '0';
  if (p_radix > 10) {
    assert(p_radix <= 36);

    if (digit < 10) {
      return digit;
    }

    // Force the 6th bit to be set to ensure ascii is lower case.
    digit = (p_ch | 0x20) - 'a' + 10;
  }

  if (digit < p_radix) {
    return digit;
  } else {
    return -1;
  }
}

static inline bool
parse_int_literal_helper(const char* p_begin, const char* p_end, uintmax_t* p_value, int p_radix)
{
  assert(p_begin != NULL && p_begin != p_end && p_value != NULL);

  bool overflow = false;
  uintmax_t digit;

  *p_value = 0;
  while (*p_begin != *p_end) {
    char ch = *p_begin++;
    if (ch == '_') { // digit separator
      continue;
    } else if ((digit = to_digit(ch, p_radix)) >= 0) {
      overflow |= safe_mul(p_value, p_radix);
      overflow |= safe_add(p_value, digit);
    } else {
      assert(0 && "invalid character in integer literal");
    }
  }

  return overflow;
}

bool
parse_dec_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value)
{
  return parse_int_literal_helper(p_begin, p_end, p_value, 10);
}

bool
parse_bin_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value)
{
  assert(p_begin != NULL && p_begin != p_end && p_value != NULL);
  assert(p_begin[0] == '0' && (p_begin[1] == 'b' || p_begin[1] == 'B'));

  p_begin += 2; // skip '0b'

  return parse_int_literal_helper(p_begin, p_end, p_value, 2);
}

bool
parse_oct_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value)
{
  assert(p_begin != NULL && p_begin != p_end && p_value != NULL);
  assert(p_begin[0] == '0' && (p_begin[1] == 'o' || p_begin[1] == 'O'));

  p_begin += 2; // skip '0o'

  return parse_int_literal_helper(p_begin, p_end, p_value, 8);
}

bool
parse_hex_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value)
{
  assert(p_begin != NULL && p_begin != p_end && p_value != NULL);
  assert(p_begin[0] == '0' && (p_begin[1] == 'x' || p_begin[1] == 'X'));

  p_begin += 2; // skip '0x'

  return parse_int_literal_helper(p_begin, p_end, p_value, 16);
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
  // TODO: Maybe find a better way to parse float literals.
  errno = 0;
  *p_value = strtod(buffer, NULL);
  bool too_big = (errno == ERANGE);

  free(buffer);
  return too_big;
}
