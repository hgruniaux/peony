#include "diag.h"

#include "hedley.h"

#include "diag_formatter.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PDiagContext g_diag_context = { .diagnostic_count = { 0 },
                                .max_errors = 0,
                                .warning_as_errors = false,
                                .fatal_errors = false,
                                .ignore_notes = false,
                                .ignore_warnings = false };


static PDiag g_current_diag = { .flushed = true };

PSourceFile* g_current_source_file = NULL;
bool g_verify_mode_enabled = false;
bool g_enable_ansi_colors = true;

static PDiagSeverity g_verify_last_diag_severity = P_DIAG_UNSPECIFIED;
static const char* g_verify_last_diag_message = NULL;
static int g_verify_fail_count = 0;

static void
verify_unexpected(void);

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

PDiag*
diag(PDiagKind p_kind)
{
  assert(g_current_diag.flushed && "a call to diag_flush() was forgotten");

  g_current_diag.kind = p_kind;
  g_current_diag.severity = g_diag_severities[p_kind];
  g_current_diag.message = g_diag_messages[p_kind];
  g_current_diag.range_count = 0;
  g_current_diag.arg_count = 0;
  g_current_diag.flushed = false;
  return &g_current_diag;
}

PDiag*
diag_at(PDiagKind p_kind, PSourceLocation p_location)
{
  PDiag* d = diag(p_kind);
  d->caret_location = p_location;
  return d;
}

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

  if (!g_verify_mode_enabled) {
    // Print source location:
    if (g_current_source_file != NULL) {
      uint32_t lineno, colno;
      p_source_location_get_lineno_and_colno(g_current_source_file, p_diag->caret_location, &lineno, &colno);
      fprintf(stdout, "%s:%d:%d: ", g_current_source_file->filename, lineno, colno);
    }

    // Print severity:
    if (g_enable_ansi_colors)
      fputs(g_diag_severity_colors[p_diag->severity], stdout);
    fputs(g_diag_severity_names[p_diag->severity], stdout);
    fputs(": ", stdout);
    if (g_enable_ansi_colors)
      fputs("\x1b[0m", stdout);
  }

  PMsgBuffer buffer;
  INIT_MSG_BUFFER(buffer);
  p_diag_format_msg(&buffer, p_diag->message, p_diag->args, p_diag->arg_count);

  if (g_verify_mode_enabled) {
    if (g_verify_last_diag_severity != P_DIAG_UNSPECIFIED)
      verify_unexpected();

    g_verify_last_diag_severity = p_diag->severity;
    g_verify_last_diag_message = malloc(sizeof(char) * sizeof(buffer.buffer));
    memcpy(g_verify_last_diag_message, buffer.buffer, sizeof(buffer.buffer));
  } else {
    fputs(buffer.buffer, stdout);
    fputs("\n", stdout);
  }

  if (g_diag_context.fatal_errors && p_diag->severity == P_DIAG_ERROR) {
    fprintf(stdout, "compilation terminated due to -Wfatal-errors.\n");
    exit(EXIT_FAILURE);
  }
}

/*
 * Verify mode interface:
 */

HEDLEY_PRINTF_FORMAT(1, 2)

static void
verify_print_fail(const char* p_msg, ...)
{
  fprintf(stdout, "FAIL: ");

  va_list ap;
  va_start(ap, p_msg);
  vfprintf(stdout, p_msg, ap);
  va_end(ap);

  fputs("\n", stdout);
}

static bool
verify_cmp_msg(const char* p_msg_begin, const char* p_msg_end)
{
  size_t last_msg_len = strlen(g_verify_last_diag_message);
  size_t expect_msg_len = p_msg_end - p_msg_begin;
  if (expect_msg_len != last_msg_len)
    return false;

  return memcmp(p_msg_begin, g_verify_last_diag_message, last_msg_len) == 0;
}

static void
verify_clean_last_diag(void)
{
  free(g_verify_last_diag_message);
  g_verify_last_diag_message = NULL;
  g_verify_last_diag_severity = P_DIAG_UNSPECIFIED;
}

static void
verify_unexpected(void)
{
  assert(g_verify_mode_enabled);

  ++g_verify_fail_count;

  verify_print_fail("unexpected %s diagnostic", g_diag_severity_names[g_verify_last_diag_severity]);
  fprintf(stdout, "    with message: {{%s}}\n", g_verify_last_diag_message);

  verify_clean_last_diag();
}

static void
verify_expect(PDiagSeverity p_severity, const char* p_msg_begin, const char* p_msg_end)
{
  assert(g_verify_mode_enabled);

  if (g_verify_last_diag_severity != p_severity) {
    ++g_verify_fail_count;
    verify_print_fail("expected %s diagnostic", g_diag_severity_names[g_verify_last_diag_severity]);
    if (g_verify_last_diag_severity != P_DIAG_UNSPECIFIED) {
      fprintf(stdout,
              "    but got %s diagnostic with message {{%s}}",
              g_diag_severity_names[p_severity],
              g_verify_last_diag_message);
    }

    verify_clean_last_diag();
    return;
  }

  if (g_verify_last_diag_message == NULL) {
  } else if (!verify_cmp_msg(p_msg_begin, p_msg_end)) {
    ++g_verify_fail_count;
    verify_print_fail("diagnostic message mismatch");
    fputs("    expected: {{", stdout);
    fwrite(p_msg_begin, sizeof(char), (p_msg_end - p_msg_begin), stdout);
    fputs("}}\n", stdout);
    fprintf(stdout, "    got: {{%s}}", g_verify_last_diag_message);
  }

  verify_clean_last_diag();
}

void
verify_expect_warning(const char* p_msg_begin, const char* p_msg_end)
{
  verify_expect(P_DIAG_WARNING, p_msg_begin, p_msg_end);
}

void
verify_expect_error(const char* p_msg_begin, const char* p_msg_end)
{
  verify_expect(P_DIAG_ERROR, p_msg_begin, p_msg_end);
}

void
verify_finalize(void)
{
  assert(g_verify_mode_enabled);

  if (g_verify_last_diag_severity != P_DIAG_UNSPECIFIED) {
    verify_unexpected();
  }

  if (g_verify_fail_count == 0) {
    fputs("SUCCESS\n", stdout);
  }
}
