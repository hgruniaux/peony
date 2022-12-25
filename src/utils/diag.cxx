#include "diag.hxx"

#include "../options.hxx"
#include "diag_formatter.hxx"

#include <hedley.h>

#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

PDiagContext g_diag_context = { .diagnostic_count = { 0 },
                                .max_errors = 0,
                                .warning_as_errors = false,
                                .fatal_errors = false,
                                .ignore_notes = false,
                                .ignore_warnings = false };

#ifdef P_DEBUG
static PDiag g_current_diag = { .debug_was_flushed = true };
#else
static PDiag g_current_diag;
#endif

PSourceLocation g_current_source_location = 0;
PSourceFile* g_current_source_file = nullptr;

static const char* g_diag_severity_names[] = {
  "unspecified", "note", "warning", "error", "fatal error",
};

static const char* g_diag_severity_colors[] = {
  "", "\x1b[1;36m", "\x1b[1;33m", "\x1b[1;31m", "\x1b[1;31m",
};

static PDiagSeverity g_diag_severities[] = {
#define ERROR(p_name, p_msg) P_DIAG_ERROR,
#define WARNING(p_name, p_msg) P_DIAG_WARNING,
#include "diag_kinds.def"
};

static const char* g_diag_messages[] = {
#define ERROR(p_name, p_msg) p_msg,
#define WARNING(p_name, p_msg) p_msg,
#include "diag_kinds.def"
};

#ifdef P_DEBUG
#undef diag
#undef diag_at
#endif

PDiag*
diag(PDiagKind p_kind)
{
#ifdef P_DEBUG
  if (!g_current_diag.debug_was_flushed) {
    fprintf(stderr,
            "internal compiler error: diag_flush() was not called for diagnostic generated at %s:%d\n",
            g_current_diag.debug_source_filename,
            g_current_diag.debug_source_line);
    abort();
  }
#endif

  g_current_diag.kind = p_kind;
  g_current_diag.severity = g_diag_severities[p_kind];
  g_current_diag.message = g_diag_messages[p_kind];
  g_current_diag.range_count = 0;
  g_current_diag.arg_count = 0;
  g_current_diag.caret_location = g_current_source_location;
  return &g_current_diag;
}

PDiag*
diag_at(PDiagKind p_kind, PSourceLocation p_location)
{
  PDiag* d = diag(p_kind);
  d->caret_location = p_location;
  return d;
}

#ifdef P_DEBUG
PDiag*
diag_debug_impl(PDiagKind p_kind, const char* p_filename, int p_line)
{
  PDiag* d = diag(p_kind);
  d->debug_was_flushed = false;
  d->debug_source_filename = p_filename;
  d->debug_source_line = p_line;
  return d;
}

PDiag*
diag_at_debug_impl(PDiagKind p_kind, PSourceLocation p_location, const char* p_filename, int p_line)
{
  PDiag* d = diag_at(p_kind, p_location);
  d->debug_was_flushed = false;
  d->debug_source_filename = p_filename;
  d->debug_source_line = p_line;
  return d;
}
#endif

void
diag_add_arg_char(PDiag* p_diag, char p_arg)
{
  assert(p_diag->arg_count < P_DIAG_MAX_ARGUMENTS);
  int arg_idx = p_diag->arg_count++;
  p_diag->args[arg_idx].type = P_DAT_CHAR;
  p_diag->args[arg_idx].value_char = p_arg;
}

void
diag_add_arg_int(PDiag* p_diag, int p_arg)
{
  assert(p_diag->arg_count < P_DIAG_MAX_ARGUMENTS);
  int arg_idx = p_diag->arg_count++;
  p_diag->args[arg_idx].type = P_DAT_INT;
  p_diag->args[arg_idx].value_int = p_arg;
}

void
diag_add_arg_str(PDiag* p_diag, const char* p_arg)
{
  assert(p_diag->arg_count < P_DIAG_MAX_ARGUMENTS);
  int arg_idx = p_diag->arg_count++;
  p_diag->args[arg_idx].type = P_DAT_STR;
  p_diag->args[arg_idx].value_str = p_arg;
}

void
diag_add_arg_tok_kind(PDiag* p_diag, PTokenKind p_token_kind)
{
  assert(p_diag->arg_count < P_DIAG_MAX_ARGUMENTS);
  int arg_idx = p_diag->arg_count++;
  p_diag->args[arg_idx].type = P_DAT_TOK_KIND;
  p_diag->args[arg_idx].value_token_kind = p_token_kind;
}

void
diag_add_arg_ident(PDiag* p_diag, struct PIdentifierInfo* p_arg)
{
  assert(p_diag->arg_count < P_DIAG_MAX_ARGUMENTS);
  int arg_idx = p_diag->arg_count++;
  p_diag->args[arg_idx].type = P_DAT_IDENT;
  p_diag->args[arg_idx].value_ident = p_arg;
}

void
diag_add_arg_type(PDiag* p_diag, struct PType* p_arg)
{
  assert(p_diag->arg_count < P_DIAG_MAX_ARGUMENTS);
  int arg_idx = p_diag->arg_count++;
  p_diag->args[arg_idx].type = P_DAT_TYPE;
  p_diag->args[arg_idx].value_type = p_arg;
}

void
diag_add_arg_type_with_name_hint(PDiag* p_diag, struct PType* p_type, struct PIdentifierInfo* p_name)
{
  assert(p_diag->arg_count < P_DIAG_MAX_ARGUMENTS);
  int arg_idx = p_diag->arg_count++;
  p_diag->args[arg_idx].type = P_DAT_TYPE_WITH_NAME_HINT;
  p_diag->args[arg_idx].value_type_with_name_hint.type = p_type;
  p_diag->args[arg_idx].value_type_with_name_hint.name = p_name;
}

void
diag_add_source_range(PDiag* p_diag, PSourceRange p_range)
{
  assert(p_diag->range_count < P_DIAG_MAX_RANGES);
  p_diag->ranges[p_diag->range_count++] = p_range;
}

void
diag_add_source_caret(PDiag* p_diag, PSourceLocation p_loc)
{
  assert(p_diag->range_count < P_DIAG_MAX_RANGES);
  p_diag->ranges[p_diag->range_count++] = (PSourceRange){ p_loc, p_loc };
}

void
diag_flush(PDiag* p_diag)
{
  if (g_diag_context.ignore_notes && p_diag->severity == P_DIAG_NOTE)
    return;
  if (g_diag_context.ignore_warnings && p_diag->severity == P_DIAG_WARNING)
    return;

  // Promote warnings -> errors if requested
  if (g_diag_context.warning_as_errors && p_diag->severity == P_DIAG_WARNING)
    p_diag->severity = P_DIAG_ERROR;

  g_diag_context.diagnostic_count[p_diag->severity]++;

#ifdef P_DEBUG
  p_diag->debug_was_flushed = true;
#endif

  // Print source location:
  if (g_current_source_file != nullptr) {
    uint32_t lineno, colno;
    p_source_location_get_lineno_and_colno(g_current_source_file, p_diag->caret_location, &lineno, &colno);

    colno -= 1;
    colno += g_options.opt_diagnostics_column_origin;

    if (g_options.opt_diagnostics_show_column) {
      fprintf(stderr, "%s:%d:%d: ", g_current_source_file->get_filename().c_str(), lineno, colno);
    } else {
      fprintf(stderr, "%s:%d: ", g_current_source_file->get_filename().c_str(), lineno);
    }
  }

  // Print severity:
  if (g_options.opt_diagnostics_color)
    fputs(g_diag_severity_colors[p_diag->severity], stderr);
  fputs(g_diag_severity_names[p_diag->severity], stderr);
  fputs(": ", stderr);
  if (g_options.opt_diagnostics_color)
    fputs("\x1b[0m", stderr);

  PMsgBuffer buffer;
  INIT_MSG_BUFFER(buffer);
  p_diag_format_msg(&buffer, p_diag->message, p_diag->args, p_diag->arg_count);

  fputs(buffer.buffer, stderr);
  fputs("\n", stderr);
  if (p_diag->range_count > 0)
    p_diag_print_source_ranges(g_current_source_file, p_diag->ranges, p_diag->range_count);

  if (g_options.opt_diagnostics_max_errors != 0 &&
      g_diag_context.diagnostic_count[P_DIAG_ERROR] >= g_options.opt_diagnostics_max_errors) {
    fprintf(stderr, "compilation terminated due to -fmax-errors=%d.\n", g_options.opt_diagnostics_max_errors);
    exit(EXIT_FAILURE);
  }

  if (g_diag_context.fatal_errors && p_diag->severity == P_DIAG_ERROR) {
    fprintf(stderr, "compilation terminated due to -Wfatal-errors.\n");
    exit(EXIT_FAILURE);
  }
}
