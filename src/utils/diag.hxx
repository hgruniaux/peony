#pragma once

#include "../token_kind.hxx"
#include "source_location.hxx"

#include <cstdint>

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
  P_DAT_CHAR,                /** @brief `char` */
  P_DAT_INT,                 /** @brief `int` */
  P_DAT_STR,                 /** @brief `const char*` (NUL-terminated) */
  P_DAT_TOK_KIND,            /** @brief `PTokenKind` */
  P_DAT_IDENT,               /** @brief `PIdentifierInfo*` */
  P_DAT_TYPE,                /** @brief `PType*` */
  P_DAT_TYPE_WITH_NAME_HINT, /** @brief `PType*` and 'PIdentifierInfo*' */
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
    struct
    {
      struct PType* type;
      struct PIdentifierInfo* name;
    } value_type_with_name_hint;
  };
} PDiagArgument;

// Maximum count of arguments that a diagnostic can have at the same time.
#define P_DIAG_MAX_ARGUMENTS 8
// Maximum count of source ranges that can be attached to a diagnostic at the same time.
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

#ifdef P_DEBUG
  // The following fields are used to print a useful error when the user forget to call diag_flush().
  bool debug_was_flushed;
  // The source m_filename (as given by __FILE__) and line (__LINE__) where the diag() function was called.
  const char* debug_source_filename;
  int debug_source_line;
#endif
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

#ifdef P_DEBUG
PDiag*
diag_debug_impl(PDiagKind p_kind, const char* p_filename, int p_line);

PDiag*
diag_at_debug_impl(PDiagKind p_kind, PSourceLocation p_location, const char* p_filename, int p_line);

#define diag(p_kind) diag_debug_impl((p_kind), __FILE__, __LINE__)
#define diag_at(p_kind, p_location) diag_at_debug_impl((p_kind), (p_location), __FILE__, __LINE__)
#else
PDiag*
diag(PDiagKind p_kind);

PDiag*
diag_at(PDiagKind p_kind, PSourceLocation p_location);
#endif

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
diag_add_arg_type_with_name_hint(PDiag* p_diag, struct PType* p_type, struct PIdentifierInfo* p_name);

void
diag_add_source_range(PDiag* p_diag, PSourceRange p_range);

void
diag_add_source_caret(PDiag* p_diag, PSourceLocation p_loc);

void
diag_flush(PDiag* p_diag);

HEDLEY_END_C_DECLS
