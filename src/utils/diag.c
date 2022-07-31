#include "diag.h"

#include "hedley.h"

#include "../identifier_table.h"
#include "../token.h"
#include "../type.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* CURRENT_FILENAME = NULL;
unsigned int CURRENT_LINENO = 0;
bool g_verify_mode_enabled = true;

static PDiagSeverity g_verify_last_diag_severity = P_DIAG_UNSPECIFIED;
static const char* g_verify_last_diag_message = NULL;
static int g_verify_fail_count = 0;

static void
verify_unexpected(void);

static bool g_enable_ansi_colors = true;

#define F_CODE_BEGIN "\x1b[1m"
#define F_CODE_END "\x1b[0m"

struct PMsgBuffer
{
  char buffer[2048];
  char* it;
};

static void
write_msg_buffer(struct PMsgBuffer* p_buffer, const char* p_text)
{
  size_t text_len = strlen(p_text);
  assert(p_buffer->it + text_len < (p_buffer->buffer + sizeof(p_buffer->buffer)));

  memcpy(p_buffer->it, p_text, sizeof(char) * text_len);
  p_buffer->it += text_len;
}

static void
print_location()
{
  /* do not print anything if we are in verify mode */
  if (g_verify_mode_enabled)
    return;

  if (CURRENT_FILENAME != NULL) {
    fprintf(stderr, "%s:%d: ", CURRENT_FILENAME, CURRENT_LINENO);
  }
}

/* Prints to stderr the given severity in the given format:
 * "severity: " (using ANSI colors optionally). */
static void
print_severity(PDiagSeverity p_severity)
{
  /* do not print anything if we are in verify mode */
  if (g_verify_mode_enabled)
    return;

  const char* severity_name;
  const char* severity_color;
  switch (p_severity) {
    case P_DIAG_NOTE:
      severity_name = "note";
      severity_color = "\x1b[1;36m";
      break;
    case P_DIAG_WARNING:
      severity_name = "warning";
      severity_color = "\x1b[1;33m";
      break;
    case P_DIAG_ERROR:
      severity_name = "error";
      severity_color = "\x1b[1;31m";
      break;
    case P_DIAG_FATAL:
      severity_name = "fatal";
      severity_color = "\x1b[1;31m";
      break;
    default:
      HEDLEY_UNREACHABLE();
  }

  if (g_enable_ansi_colors) {
    fprintf(stderr, "%s%s:\x1b[0m ", severity_color, severity_name);
  } else {
    fprintf(stderr, "%s: ", severity_name);
  }
}

static void
format_type(struct PMsgBuffer* p_buffer, PType* p_type)
{
  /* WARNING: Must have the same layout as PTypeKind. */
  static const char* builtin_types[] = { "void", "undef", "char", "i8",        "i16", "i32", "i64",     "u8",
                                         "u16",  "u32",   "u64",  "{integer}", "f32", "f64", "{float}", "bool" };

  if (P_TYPE_GET_KIND(p_type) == P_TYPE_FUNCTION) {
    PFunctionType* func_type = (PFunctionType*)p_type;
    write_msg_buffer(p_buffer, "fn (");
    for (int i = 0; i < func_type->arg_count; ++i) {
      format_type(p_buffer, func_type->args[i]);
      if ((i + 1) != func_type->arg_count)
        write_msg_buffer(p_buffer, ", ");
    }
    write_msg_buffer(p_buffer, "): ");
    format_type(p_buffer, func_type->ret_type);
  } else if (P_TYPE_GET_KIND(p_type) == P_TYPE_POINTER) {
    PPointerType* ptr_type = (PPointerType*)p_type;
    write_msg_buffer(p_buffer, "*");
    format_type(p_buffer, ptr_type->element_type);
  } else if (P_TYPE_GET_KIND(p_type) == P_TYPE_PAREN) {
    PParenType* paren_type = (PParenType*)p_type;
    format_type(p_buffer, paren_type->sub_type);
  } else {
    write_msg_buffer(p_buffer, builtin_types[(int)P_TYPE_GET_KIND(p_type)]);
  }
}

static void
format_msg(struct PMsgBuffer* p_buffer, const char* p_msg, va_list p_ap)
{
  for (const char* it = p_msg; *it != '\0'; ++it) {
    if (*it == '<' && it[1] == '%') {
      write_msg_buffer(p_buffer, F_CODE_BEGIN);
      it += 1;
    } else if (*it == '%' && it[1] == '>') {
      write_msg_buffer(p_buffer, F_CODE_END);
      it += 1;
    } else if (*it == '%' && it[1] == 'i') { /* '%i' */
      PIdentifierInfo* ident = va_arg(p_ap, PIdentifierInfo*);
      assert(ident != NULL);
      write_msg_buffer(p_buffer, ident->spelling);
      it += 1;
    } else if (*it == '%' && it[1] == 't') {
      if (it[2] == 'k') {
        PTokenKind tok_kind = va_arg(p_ap, PTokenKind);

        const char* spelling = p_token_kind_get_spelling(tok_kind);
        if (spelling == NULL)
          spelling = p_token_kind_get_name(tok_kind);

        write_msg_buffer(p_buffer, spelling);
      } else if (it[2] == 'y') {
        PType* type = va_arg(p_ap, PType*);
        assert(type != NULL);
        format_type(p_buffer, type);
      }

      it += 2;
    } else if (*it == '%' && it[1] == 's') {
      const char* str = va_arg(p_ap, const char*);
      write_msg_buffer(p_buffer, str);
      it += 1;
    } else if (*it == '%' && it[1] == 'd') {
      int value = va_arg(p_ap, int);
      int written_bytes = sprintf(p_buffer->buffer, "%d", value);
      p_buffer->it += written_bytes;
      assert(p_buffer->it < (p_buffer->buffer + sizeof(p_buffer->buffer)));
      it += 1;
    } else if (*it == '%' && it[1] == 'c') {
      char value = va_arg(p_ap, char);
      if (isprint(value)) {
        *(p_buffer->it++) = *it;
        assert(p_buffer->it < (p_buffer->buffer + sizeof(p_buffer->buffer)));
      } else {
        int written_bytes = sprintf(p_buffer->buffer, "\\x%x", (int)value);
        p_buffer->it += written_bytes;
        assert(p_buffer->it < (p_buffer->buffer + sizeof(p_buffer->buffer)));
      }
      it += 1;
    } else {
      *(p_buffer->it++) = *it;
      assert(p_buffer->it < (p_buffer->buffer + sizeof(p_buffer->buffer)));
    }
  }

  fputs("\n", stderr);
}

static void
print_msg(const char* p_msg, va_list p_ap)
{
  struct PMsgBuffer buffer;
  buffer.it = buffer.buffer;
  format_msg(&buffer, p_msg, p_ap);
  *buffer.it = '\0';

  if (g_verify_mode_enabled) {
    g_verify_last_diag_message = malloc(sizeof(char) * sizeof(buffer.buffer));
    memcpy(g_verify_last_diag_message, buffer.buffer, sizeof(buffer.buffer));
  } else {
    fputs(buffer.buffer, stderr);
  }
}

static void
diag_impl(PDiagSeverity p_severity, const char* p_msg, va_list p_ap)
{
  if (!g_verify_mode_enabled) {
    print_location();
    print_severity(p_severity);
  } else {
    if (g_verify_last_diag_severity != P_DIAG_UNSPECIFIED)
      verify_unexpected();

    g_verify_last_diag_severity = p_severity;
  }

  print_msg(p_msg, p_ap);
}

void
note(const char* p_msg, ...)
{
  va_list ap;
  va_start(ap, p_msg);
  diag_impl(P_DIAG_NOTE, p_msg, ap);
  va_end(ap);
}

void
warning(const char* p_msg, ...)
{
  va_list ap;
  va_start(ap, p_msg);
  diag_impl(P_DIAG_WARNING, p_msg, ap);
  va_end(ap);
}

void
error(const char* p_msg, ...)
{
  va_list ap;
  va_start(ap, p_msg);
  diag_impl(P_DIAG_ERROR, p_msg, ap);
  va_end(ap);
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

static const char*
verify_get_severity_name(PDiagSeverity p_severity)
{
  switch (p_severity) {
    case P_DIAG_NOTE:
      return "NOTE";
    case P_DIAG_WARNING:
      return "WARNING";
    case P_DIAG_ERROR:
      return "ERROR";
    case P_DIAG_FATAL:
      return "FATAL";
    case P_DIAG_UNSPECIFIED:
    default:
      return "UNSPECIFIED";
  }
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

  verify_print_fail("unexpected %s diagnostic", verify_get_severity_name(g_verify_last_diag_severity));
  fprintf(stdout, "    with message: {{%s}}\n", g_verify_last_diag_message);

  verify_clean_last_diag();
}

static void
verify_expect(PDiagSeverity p_severity, const char* p_msg_begin, const char* p_msg_end)
{
  assert(g_verify_mode_enabled);

  if (g_verify_last_diag_severity != p_severity) {
    ++g_verify_fail_count;
    verify_print_fail("expected %s diagnostic", verify_get_severity_name(p_severity));
    if (g_verify_last_diag_severity != P_DIAG_UNSPECIFIED) {
      fprintf(stdout,
              "    but got %s diagnostic with message {{%s}}",
              verify_get_severity_name(p_severity),
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
