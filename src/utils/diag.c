#include "diag.h"

#include "../hedley.h"

#include "../identifier_table.h"
#include "../token.h"
#include "../type.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

const char* CURRENT_FILENAME = NULL;
unsigned int CURRENT_LINENO = 0;

static bool g_enable_ansi_colors = true;

#define F_CODE_BEGIN "\x1b[1m"
#define F_CODE_END "\x1b[0m"

static void
print_location()
{
  if (CURRENT_FILENAME != NULL) {
    fprintf(stderr, "%s:%d: ", CURRENT_FILENAME, CURRENT_LINENO);
  }
}

/* Prints to stderr the given severity in the given format:
 * "severity: " (using ANSI colors optionally). */
static void
print_severity(PDiagSeverity p_severity)
{
  const char* severity_name;
  const char* severity_color;
  switch (p_severity) {
    case P_DIAG_NOTE:
      severity_name = "note";
      severity_color = "\x1b[1;36m";
      break;
    case P_DIAG_WARN:
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
print_type(PType* p_type)
{
  static const char* builtin_types[] = { "void", "undef", "i8",  "i16", "i32", "i64", "u8",
                                         "u16",  "u32",   "u64", "f32", "f64", "bool" };

  if (P_TYPE_GET_KIND(p_type) == P_TYPE_FUNCTION) {
    PFunctionType* func_type = (PFunctionType*)p_type;
    fputs("fn (", stderr);
    for (int i = 0; i < func_type->arg_count; ++i) {
      print_type(func_type->args[i]);
      if ((i + 1) != func_type->arg_count)
        puts(", ");
    }
    fputs("): ", stderr);
    print_type(func_type->ret_type);
  } else {
    fputs(builtin_types[(int)P_TYPE_GET_KIND(p_type)], stderr);
  }
}

static void
print_msg(const char* p_msg, va_list p_ap)
{
  for (const char* it = p_msg; *it != '\0'; ++it) {
    if (*it == '<' && it[1] == '%') {
      fputs(F_CODE_BEGIN, stderr);
      it += 1;
    } else if (*it == '%' && it[1] == '>') {
      fputs(F_CODE_END, stderr);
      it += 1;
    } else if (*it == '%' && it[1] == 'i') { /* '%i' */
      PIdentifierInfo* ident = va_arg(p_ap, PIdentifierInfo*);
      assert(ident != NULL);
      fputs(ident->spelling, stderr);
      it += 1;
    } else if (*it == '%' && it[1] == 't') {
      if (it[2] == 'k') {
        PTokenKind tok_kind = va_arg(p_ap, PTokenKind);

        const char* spelling = p_token_kind_get_spelling(tok_kind);
        if (spelling == NULL)
          spelling = p_token_kind_get_name(tok_kind);

        fputs(spelling, stderr);
      } else if (it[2] == 'y') {
        PType* type = va_arg(p_ap, PType*);
        assert(type != NULL);
        print_type(type);
      }

      it += 2;
    } else if (*it == '%' && it[1] == 's') {
      const char* str = va_arg(p_ap, const char*);
      fputs(str, stderr);
      it += 1;
    } else if (*it == '%' && it[1] == 'd') {
      int value = va_arg(p_ap, int);
      fprintf(stderr, "%d", value);
      it += 1;
    } else if (*it == '%' && it[1] == 'c') {
      char value = va_arg(p_ap, char);
      if (isprint(value)) {
        putc(value, stderr);
      } else {
        fprintf(stderr, "\\x%x", (int)value);
      }
      it += 1;
    } else {
      putc(*it, stderr);
    }
  }

  fputs("\n", stderr);
}

void
note(const char* p_msg, ...)
{
  print_location();
  print_severity(P_DIAG_NOTE);

  va_list ap;
  va_start(ap, p_msg);
  print_msg(p_msg, ap);
  va_end(ap);
}

void
warning(const char* p_msg, ...)
{
  print_location();
  print_severity(P_DIAG_WARN);

  va_list ap;
  va_start(ap, p_msg);
  print_msg(p_msg, ap);
  va_end(ap);
}

void
error(const char* p_msg, ...)
{
  print_location();
  print_severity(P_DIAG_ERROR);

  va_list ap;
  va_start(ap, p_msg);
  print_msg(p_msg, ap);
  va_end(ap);
}
