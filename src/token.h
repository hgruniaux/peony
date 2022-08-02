#pragma once

#include "token_kind.h"
#include "utils/source_location.h"

HEDLEY_BEGIN_C_DECLS

struct PIdentifierInfo;

// The different possible integer literal suffixes.
typedef enum PIntLiteralSuffix
{
  P_ILS_NO_SUFFIX, // no suffix, e.g. '42'
  P_ILS_I8,        // e.g. '42i8'
  P_ILS_I16,       // e.g. '42i16'
  P_ILS_I32,       // e.g. '42i32'
  P_ILS_I64,       // e.g. '42i64'
  P_ILS_U8,        // e.g. '42u8'
  P_ILS_U16,       // e.g. '42u16'
  P_ILS_U32,       // e.g. '42u32'
  P_ILS_U64        // e.g. '42u64'
} PIntLiteralSuffix;

// The different possible float literal suffixes.
typedef enum PFloatLiteralSuffix
{
  P_FLS_NO_SUFFIX, // no suffix, e.g. '3.14'
  P_FLS_F32,       // e.g. '3.14f32'
  P_FLS_F64,       // e.g. '3.14f64'
} PFloatLiteralSuffix;

typedef struct PTokenLiteralData
{
  // The radix of the integer literal (either 2, 10 or 16).
  // This value is undefined for tokens other than P_TOK_INT_LITERAL.
  int int_radix;
  // Either a value of PIntLiteralSuffix or PFloatLiteralSuffix depending
  // on the token kind.
  int suffix_kind;
  // begin and end are pointers to the source file buffer used
  // by the lexer and therefore their lifetime depends on that of the file.
  // The range [begin, end) does not include any integer or float
  // suffix (e.g. `f32`).
  const char* begin;
  const char* end;
} PTokenLiteralData;

union PTokenData
{
  // This field is only valid for P_TOK_IDENTIFIER and P_TOK_KEY_* tokens.
  struct PIdentifierInfo* identifier;
  // This field is only valid for P_TOK_*_LITERAL and P_TOK_COMMENT tokens.
  // P_TOK_COMMENT tokens use literal.begin and literal.end to store the
  // comment content.
  PTokenLiteralData literal;
};

typedef struct PToken
{
  PTokenKind kind;

  // Source location pointing to the first character of the token. To get the
  // end source location just use 'source_location + token_length'.
  PSourceLocation source_location;
  // Number of characters (bytes) over which the token is spread in the source code.
  uint32_t token_length;

  // Store additional data needed for some types.
  union PTokenData data;
} PToken;

const char*
p_get_token_spelling(PToken* p_token);

/* Dumps token info to stdout for debugging purposes. */
void
p_token_dump(PToken* p_token);

HEDLEY_END_C_DECLS
