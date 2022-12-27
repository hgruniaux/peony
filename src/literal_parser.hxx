#pragma once

#include <hedley.h>

#include <cstdint>

/*
 * This site file contains a set of functions that allows to extract the value
 * of the literal returned by the lexical analyzer (this one just returns two
 * pointers marking the beginning and the end of the literal but not its
 * calculated value).
 *
 * All the functions below assume that the inputs they receive are correct and
 * validated by the lexical analyzer.
 */

bool
parse_int_literal_token(const char* p_begin, const char* p_end, int p_radix, uintmax_t& p_value);

/// Parse a float literal (possibly with digit separators).
///
/// Returns true if the value is too big to hold in p_value, otherwise returns
/// false. If true is returned, the value in p_value is undefined.
bool
parse_float_literal_token(const char* p_begin, const char* p_end, double& p_value);
