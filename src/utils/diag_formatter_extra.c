#include "diag_formatter.h"

#include "../identifier_table.h"
#include "../type.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#define DIAG_MAX_LINE_LENGTH 5

/* Format function for P_DAT_CHAR. */
static void
format_arg_char(PMsgBuffer* p_buffer, char p_value)
{
  if (isprint(p_value)) {
    write_buffer(p_buffer, &p_value, sizeof(p_value));
  } else {
    write_buffer_printf(p_buffer, "\\x%02x", (int)p_value);
  }
}

/* Format function for P_DAT_INT. */
static void
format_arg_int(PMsgBuffer* p_buffer, int p_value)
{
  write_buffer_printf(p_buffer, "%d", p_value);
}

/* Format function for P_DAT_STR. */
static void
format_arg_str(PMsgBuffer* p_buffer, const char* p_str)
{
  assert(p_str != NULL);
  write_buffer_str(p_buffer, p_str);
}

/* Format function for P_DAT_TOK_KIND. */
static void
format_arg_tok_kind(PMsgBuffer* p_buffer, PTokenKind p_token_kind)
{
  const char* spelling = p_token_kind_get_spelling(p_token_kind);
  if (spelling == NULL)
    spelling = p_token_kind_get_name(p_token_kind);
  write_buffer_str(p_buffer, spelling);
}

/* Format function for P_DAT_IDENT. */
static void
format_arg_ident(PMsgBuffer* p_buffer, PIdentifierInfo* p_ident)
{
  assert(p_ident != NULL);
  write_buffer(p_buffer, p_ident->spelling, p_ident->spelling_len);
}

/* Format function for P_DAT_TYPE. */
static void
format_arg_type(PMsgBuffer* p_buffer, PType* p_type)
{
  assert(p_type != NULL);

  /* WARNING: Must have the same layout as PTypeKind. */
  static const char* builtin_types[] = { "void", "undef", "char", "i8",        "i16", "i32", "i64",     "u8",
                                         "u16",  "u32",   "u64",  "{integer}", "f32", "f64", "{float}", "bool" };

  if (P_TYPE_GET_KIND(p_type) == P_TYPE_FUNCTION) {
    PFunctionType* func_type = (PFunctionType*)p_type;
    write_buffer_str(p_buffer, "fn (");
    for (int i = 0; i < func_type->arg_count; ++i) {
      format_arg_type(p_buffer, func_type->args[i]);
      if ((i + 1) != func_type->arg_count)
        write_buffer_str(p_buffer, ", ");
    }
    write_buffer_str(p_buffer, ") -> ");
    format_arg_type(p_buffer, func_type->ret_type);
  } else if (P_TYPE_GET_KIND(p_type) == P_TYPE_POINTER) {
    PPointerType* ptr_type = (PPointerType*)p_type;
    write_buffer_str(p_buffer, "*");
    format_arg_type(p_buffer, ptr_type->element_type);
  } else if (P_TYPE_GET_KIND(p_type) == P_TYPE_PAREN) {
    PParenType* paren_type = (PParenType*)p_type;
    format_arg_type(p_buffer, paren_type->sub_type);
  } else {
    write_buffer_str(p_buffer, builtin_types[(int)P_TYPE_GET_KIND(p_type)]);
  }
}

void
p_diag_format_arg(PMsgBuffer* p_buffer, PDiagArgument* p_arg)
{
  assert(p_arg != NULL);

  switch (p_arg->type) {
    case P_DAT_CHAR:
      format_arg_char(p_buffer, p_arg->value_char);
      break;
    case P_DAT_INT:
      format_arg_int(p_buffer, p_arg->value_int);
      break;
    case P_DAT_STR:
      format_arg_str(p_buffer, p_arg->value_str);
      break;
    case P_DAT_TOK_KIND:
      format_arg_tok_kind(p_buffer, p_arg->value_token_kind);
      break;
    case P_DAT_IDENT:
      format_arg_ident(p_buffer, p_arg->value_ident);
      break;
    case P_DAT_TYPE:
      format_arg_type(p_buffer, p_arg->value_type);
      break;
    default:
      HEDLEY_UNREACHABLE();
  }
}

void
p_diag_print_source_line(PSourceFile* p_file, uint32_t p_lineno)
{
  assert(p_file != NULL);

  uint32_t start_position = p_line_map_get_line_start_position(&p_file->line_map, p_lineno);

  size_t line_length = 0;
  const char* it = p_file->buffer + start_position;
  while (*it != '\0' && *it != '\n' && *it != '\r') {
    ++line_length;
    ++it;
  }

  fprintf(stdout, "%5d | ", p_lineno);
  fwrite(p_file->buffer + start_position, sizeof(char), line_length, stdout);
}
