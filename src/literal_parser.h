#pragma once

#include <hedley.h>

#include <stdbool.h>
#include <stdint.h>

HEDLEY_BEGIN_C_DECLS

/*
 * This site file contains a set of functions that allows to extract the value
 * of the literal returned by the lexical analyzer (this one just returns two
 * pointers marking the beginning and the end of the literal but not its
 * calculated value).
 *
 * All the functions below assume that the inputs they receive are correct and
 * validated by the lexical analyzer.
 */

/*
 * Parse a decimal integer literal (possibly with digit separators).
 *
 * If an overflow occurs this function returns true, otherwise false is
 * returned. In case of overflow, the value of p_value is undefined.
 */
bool
parse_dec_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value);

/*
 * Same as parse_dec_int_literal_token() but takes a binary integer literal
 * INCLUDING the '0b' prefix.
 */
bool
parse_bin_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value);

/*
 * Same as parse_dec_int_literal_token() but takes an octal integer literal
 * INCLUDING the '0o' prefix.
 */
bool
parse_oct_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value);

/*
 * Same as parse_dec_int_literal_token() but takes a hexadecimal integer literal
 * INCLUDING the '0x' prefix.
 */
bool
parse_hex_int_literal_token(const char* p_begin, const char* p_end, uintmax_t* p_value);

/*
 * Parse a float literal (possibly with digit separators).
 *
 * Returns true if the value is too big to hold in p_value, otherwise returns
 * false. If true is returned, the value in p_value is undefined.
 */
bool
parse_float_literal_token(const char* p_begin, const char* p_end, double* p_value);

HEDLEY_END_C_DECLS
