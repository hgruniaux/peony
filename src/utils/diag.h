#pragma once

#include "../token_kind.h"
#include "source_location.h"

#include <stdbool.h>
#include <stdint.h>

HEDLEY_BEGIN_C_DECLS

typedef enum PDiagSeverity
{
  P_DIAG_UNSPECIFIED,
  P_DIAG_NOTE,
  P_DIAG_WARNING,
  P_DIAG_ERROR,
  P_DIAG_FATAL,

  P_DIAG_SEVERITY_LAST
} PDiagSeverity;

typedef enum PDiagKind
{
#define ERROR(p_name, p_msg) P_DK_err_##p_name,
#define WARNING(p_name, p_msg) P_DK_warn_##p_name,
#include "diag_kinds.def"
} PDiagKind;

typedef enum PDiagArgumentType
{
  P_DAT_CHAR,     /** @brief `char` */
  P_DAT_INT,      /** @brief `int` */
  P_DAT_STR,      /** @brief `const char*` (NUL-terminated) */
  P_DAT_TOK_KIND, /** @brief `PTokenKind` */
  P_DAT_IDENT,    /** @brief `PIdentifierInfo*` */
  P_DAT_TYPE,     /** @brief `PType*` */
} PDiagArgumentType;

typedef struct PDiagArgument
{
  PDiagArgumentType type;
  union
  {
    char value_char;
    int value_int;
    const char* value_str;
    PTokenKind value_token_kind;
    struct PIdentifierInfo* value_ident;
    struct PType* value_type;
  };
} PDiagArgument;

#define P_DIAG_MAX_ARGUMENTS 8
#define P_DIAG_MAX_RANGES 4

typedef struct PDiag
{
  PDiagKind kind;
  PDiagSeverity severity;
  const char* message;
  PSourceLocation caret_location;
  PSourceRange ranges[P_DIAG_MAX_RANGES];
  uint32_t range_count;
  PDiagArgument args[P_DIAG_MAX_ARGUMENTS];
  uint32_t arg_count;
  bool flushed;
} PDiag;

typedef struct PDiagContext
{
  int diagnostic_count[P_DIAG_SEVERITY_LAST];
  int max_errors;
  bool warning_as_errors;
  bool fatal_errors;
  bool ignore_notes;
  bool ignore_warnings;
} PDiagContext;

extern PDiagContext g_diag_context;

extern PSourceLocation g_current_source_location;
extern PSourceFile* g_current_source_file;
extern bool g_verify_mode_enabled;
extern bool g_enable_ansi_colors;

PDiag*
diag(PDiagKind p_kind);

PDiag*
diag_at(PDiagKind p_kind, PSourceLocation p_location);

void
diag_add_arg_char(PDiag* p_diag, char p_arg);

void
diag_add_arg_int(PDiag* p_diag, int p_arg);

void
diag_add_arg_str(PDiag* p_diag, const char* p_arg);

void
diag_add_arg_tok_kind(PDiag* p_diag, PTokenKind p_token_kind);

void
diag_add_arg_ident(PDiag* p_diag, struct PIdentifierInfo* p_arg);

void
diag_add_arg_type(PDiag* p_diag, struct PType* p_arg);

void
diag_add_source_range(PDiag* p_diag, PSourceRange p_range);

void
diag_flush(PDiag* p_diag);

/* Verify mode interface (used for testsuite): */

void
verify_expect_warning(const char* p_msg_begin, const char* p_msg_end);

void
verify_expect_error(const char* p_msg_begin, const char* p_msg_end);

void
verify_finalize(void);

HEDLEY_END_C_DECLS
