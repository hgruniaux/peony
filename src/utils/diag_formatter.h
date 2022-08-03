#pragma once

#include "diag.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

HEDLEY_BEGIN_C_DECLS

typedef struct PMsgBuffer
{
  char buffer[2048];
  char* it;
  char* end;
} PMsgBuffer;

#define INIT_MSG_BUFFER(p_buffer)                                                                                      \
  ((p_buffer).it = (p_buffer).buffer);                                                                                 \
  (p_buffer).end = (p_buffer).buffer + sizeof((p_buffer).buffer)

static void
write_buffer(PMsgBuffer* p_buffer, const char* p_bytes, size_t p_size)
{
  assert((p_buffer->it + p_size) < p_buffer->end);
  memcpy(p_buffer->it, p_bytes, p_size);
  p_buffer->it += p_size;
}

static void
write_buffer_str(PMsgBuffer* p_buffer, const char* p_str)
{
  write_buffer(p_buffer, p_str, strlen(p_str));
}

HEDLEY_PRINTF_FORMAT(2, 3)
static void
write_buffer_printf(PMsgBuffer* p_buffer, const char* p_format, ...)
{
  va_list ap;
  va_start(ap, p_format);
  int written_bytes = vsprintf(p_buffer->it, p_format, ap);
  assert(p_buffer->it + written_bytes < p_buffer->end);
  p_buffer->it += written_bytes;
  va_end(ap);
}

void
p_diag_format_arg(PMsgBuffer* p_buffer, PDiagArgument* p_arg);

void
p_diag_format_msg(PMsgBuffer* p_buffer, const char* p_msg, PDiagArgument* p_args, size_t p_arg_count);

/* Prints the given set of source ranges along the source lines to stderr. */
void
p_diag_print_source_ranges(PSourceFile* p_file, PSourceRange* p_ranges, size_t p_range_count);

/* Prints a source line from the given file to stderr in the form:
 *     4 | fn foo() -> i32 {
 *
 * Returns the line length in bytes.
 */
size_t
p_diag_print_source_line(PSourceFile* p_file, uint32_t p_lineno);

HEDLEY_END_C_DECLS
