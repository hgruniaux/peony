#ifndef PEONY_LITERAL_PARSER_HXX
#define PEONY_LITERAL_PARSER_HXX

#include <cstdint>
#include <string>

// This file contains a set of functions that allows to extract the value
// of the literal returned by the lexical analyzer (this one just returns two
// pointers marking the beginning and the end of the literal but not its
// calculated value).
//
// All the functions below assume that the inputs they receive are correct and
// validated by the lexical analyzer.

/// Parse an integer literal (possibly with digit separators).
///
/// The input is expected to exactly represents a valid integer literal
/// as tokenized by the lexer. Moreover, the input is expected to contain
/// the initial radix prefix (e.g. '0b'). However, the final integer
/// suffix (e.g. 'i32') must not be included.
///
/// Because Peony only supports binary, octal, decimal and hexadecimal
/// literals, p_radix must be one of [2, 8, 10, 16].
///
/// Returns true if the parsed value is too big to hold in p_value, otherwise
/// returns false. If true is returned, the returned is undefined.
bool
parse_int_literal_token(const char* p_begin, const char* p_end, int p_radix, uintmax_t& p_value);

/// Parse a float literal (possibly with digit separators).
///
/// The input is expected to exactly represents a valid floating-point literal
/// as tokenized by the lexer. However, the final floating-point suffix (e.g. 'f32')
/// must not be included.
///
/// Returns true if the parsed value is too big to hold in p_value, otherwise
/// returns false. If true is returned, the returned is undefined.
bool
parse_float_literal_token(const char* p_begin, const char* p_end, double& p_value);

/// Parse a string literal resolving all escape sequences.
///
/// The input is expected to exactly represents a valid string literal as
/// tokenized by the lexer. Moreover, the input is expected to contain the
/// initial and final string quote i.e. ".
///
/// This function will return the string literal value UTF-8 encoded (all
/// escape sequences will have been already expanded to their value).
std::string
parse_string_literal_token(const char* p_begin, const char* p_end);

#endif // PEONY_LITERAL_PARSER_HXX
