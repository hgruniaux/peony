#include "ast/ast.hxx"
#include "diag_formatter.hxx"

#include "../options.hxx"

#include <cctype>
#include <charconv>
#include <cstdio>

/* Format function for P_DAT_CHAR. */
static void
format_arg_char(std::string& p_buffer, char p_value)
{
  if (isprint(p_value)) {
    p_buffer.push_back(p_value);
  } else {
    // FIXME
    p_buffer.append("\\x");

    unsigned second_digit = p_value % 16;
    unsigned first_digit = p_value / 16;

    if (first_digit > 9)
      p_buffer.push_back('a' + (first_digit - 10));
    else
      p_buffer.push_back('0' + first_digit);

    if (second_digit > 9)
      p_buffer.push_back('a' + (second_digit - 10));
    else
      p_buffer.push_back('0' + second_digit);
  }
}

/* Format function for P_DAT_INT. */
static void
format_arg_int(std::string& p_buffer, int p_value)
{
  p_buffer.append(std::to_string(p_value));
}

/* Format function for P_DAT_STR. */
static void
format_arg_str(std::string& p_buffer, const char* p_str)
{
  assert(p_str != nullptr);
  p_buffer.append(p_str);
}

/* Format function for P_DAT_TOK_KIND. */
static void
format_arg_tok_kind(std::string& p_buffer, PTokenKind p_token_kind)
{
  const char* spelling = token_kind_get_spelling(p_token_kind);
  if (spelling == nullptr)
    spelling = token_kind_get_name(p_token_kind);
  p_buffer.append(spelling);
}

/* Format function for P_DAT_IDENT. */
static void
format_arg_ident(std::string& p_buffer, PIdentifierInfo* p_ident)
{
  assert(p_ident != nullptr);
  p_buffer.append(p_ident->spelling, p_ident->spelling + p_ident->spelling_len);
}

/* Format function for P_DAT_TYPE and P_DAT_TYPE_WITH_NAME_HINT. */
static void
format_arg_type(std::string& p_buffer, PType* p_type, PIdentifierInfo* p_name_hint)
{
  assert(p_type != nullptr);

  if (p_type->get_kind() == P_TK_FUNCTION) {
    auto* func_type = p_type->as<PFunctionType>();
    p_buffer.append("fn ");
    if (p_name_hint != nullptr)
      p_buffer.append(p_name_hint->spelling);

    p_buffer.append("(");
    for (size_t i = 0; i < func_type->get_param_count(); ++i) {
      format_arg_type(p_buffer, func_type->get_params()[i], nullptr); // do not propagate the hint
      if ((i + 1) != func_type->get_param_count())
        p_buffer.append(", ");
    }

    if (func_type->get_ret_ty()->is_void_ty()) {
      p_buffer.append(")");
    } else {
      p_buffer.append(") -> ");
      format_arg_type(p_buffer, func_type->get_ret_ty(), nullptr);
    }
  } else if (p_type->get_kind() == P_TK_POINTER) {
    auto* ptr_type = p_type->as<PPointerType>();
    p_buffer.append("*");
    format_arg_type(p_buffer, ptr_type->get_element_ty(), nullptr); // do not propagate the hint
  } else if (p_type->get_kind() == P_TK_PAREN) {
    auto* paren_type = p_type->as<PParenType>();
    format_arg_type(p_buffer, paren_type->get_sub_type(), p_name_hint);
  } else {
    switch (p_type->get_kind()) {
#define TYPE(p_kind)
#define BUILTIN_TYPE(p_kind, p_spelling) case p_kind: p_buffer.append(p_spelling); break;
#include "type.def"
      default:
        assert(false && "not a builtin type");
    }
  }
}

void
p_diag_format_arg(std::string& p_buffer, PDiagArgument* p_arg)
{
  assert(p_arg != nullptr);

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
      format_arg_type(p_buffer, p_arg->value_type, /* no hint */ nullptr);
      break;
    case P_DAT_TYPE_WITH_NAME_HINT:
      format_arg_type(p_buffer, p_arg->value_type_with_name_hint.type, p_arg->value_type_with_name_hint.name);
      break;
    default:
      HEDLEY_UNREACHABLE();
  }
}

// A source range which is spread out only on a single line.
// A classic source range is decomposed into several partial source
// ranges if it spans more than one line.
struct PartialSourceRange
{
  uint32_t lineno;
  uint32_t colno_begin;
  uint32_t colno_end;
};

#define CREATE_PARTIAL_SRC_RANGE(p_lineno, p_colno_begin, p_colno_end)                                                 \
  (struct PartialSourceRange)                                                                                          \
  {                                                                                                                    \
    .lineno = (p_lineno), .colno_begin = (p_colno_begin), .colno_end = (p_colno_end)                                   \
  }

static int
partial_src_range_cmp(const void* p_lhs, const void* p_rhs)
{
  struct PartialSourceRange lhs = *(const struct PartialSourceRange*)p_lhs;
  struct PartialSourceRange rhs = *(const struct PartialSourceRange*)p_rhs;

  if (lhs.lineno < rhs.lineno)
    return -1;
  if (lhs.lineno > rhs.lineno)
    return 1;

  if (lhs.colno_begin < rhs.colno_begin)
    return -1;
  if (lhs.colno_begin > rhs.colno_begin)
    return 1;

  if (lhs.colno_end < rhs.colno_end)
    return -1;

  // if (lhs.colno_end > rhs.colno_end)
  return 1;
}

static void
print_char_n_times(char p_c, int p_n)
{
  // FIXME: probably find a better way
  while (p_n--)
    fputc(p_c, stderr);
}

static void
print_partial_src_range(struct PartialSourceRange* p_range, uint32_t p_current_colno)
{
  if (p_current_colno != p_range->colno_begin)
    print_char_n_times(' ', p_range->colno_begin - p_current_colno);

  if (p_range->colno_begin == p_range->colno_end)
    fputs("^", stderr);
  else
    print_char_n_times('~', p_range->colno_end - p_range->colno_begin);
}

/* Print a line margin to stderr of the form:
 * "    5 | "
 * If p_lineno is set to 0, no line number will be printed.
 * This function follows the formatting options given by user (cmd line options). */
static void
print_line_margin(uint32_t p_lineno)
{
  if (!g_options.opt_diagnostics_show_line_numbers)
    return;

  int margin_width = g_options.opt_diagnostics_minimum_margin_width;
  if (margin_width < 2)
    margin_width = 2;
  if (p_lineno == 0) {
    print_char_n_times(' ', margin_width - 1);
  } else {
    // FIXME: (char)('0' + (margin_width - 1)) is dangerous, for example if margin_width > 9
    char format[] = { '%', (char)('0' + (margin_width - 1)), 'd', '\0' };
    fprintf(stderr, format, p_lineno);
  }
  fputs(" | ", stderr);
}

void
p_diag_print_source_ranges(PSourceFile* p_file, PSourceRange* p_ranges, size_t p_range_count)
{
#define MAX_PARTIAL_SRC_RANGES 512
  struct PartialSourceRange partial_source_ranges[MAX_PARTIAL_SRC_RANGES];
  size_t partial_source_range_count = 0;

  // Decompose input source ranges into partial source ranges which are more
  // easier to handle.
  for (size_t i = 0; i < p_range_count; ++i) {
    assert(p_ranges[i].begin <= p_ranges[i].end);
    uint32_t lineno_begin, colno_begin;
    uint32_t lineno_end, colno_end;
    p_source_location_get_lineno_and_colno(p_file, p_ranges[i].begin, &lineno_begin, &colno_begin);
    p_source_location_get_lineno_and_colno(p_file, p_ranges[i].end, &lineno_end, &colno_end);

    if (partial_source_range_count + (lineno_end - lineno_begin + 1) > MAX_PARTIAL_SRC_RANGES)
      return;

    if (lineno_begin == lineno_end) {
      partial_source_ranges[partial_source_range_count++] =
        CREATE_PARTIAL_SRC_RANGE(lineno_begin, colno_begin, colno_end);
    } else {
      partial_source_ranges[partial_source_range_count++] = CREATE_PARTIAL_SRC_RANGE(lineno_begin, colno_begin, 0);
      partial_source_ranges[partial_source_range_count++] = CREATE_PARTIAL_SRC_RANGE(lineno_end, 1, colno_end);

      for (uint32_t lineno = lineno_begin + 1; lineno < lineno_end; ++lineno) {
        partial_source_ranges[partial_source_range_count++] = CREATE_PARTIAL_SRC_RANGE(lineno, 1, 0);
      }
    }
  }

  // Sort partial source ranges so the first (first in source code) source ranges appears first.
  qsort(partial_source_ranges, partial_source_range_count, sizeof(struct PartialSourceRange), partial_src_range_cmp);

  // Compress and display partial source ranges.
  uint32_t prev_lineno = partial_source_range_count > 0 ? (partial_source_ranges[0].lineno - 1) : 0;
  for (size_t i = 0; i < partial_source_range_count; ++i) {
    uint32_t current_lineno = partial_source_ranges[i].lineno;
    if (current_lineno != (prev_lineno + 1)) {
      print_line_margin(0);
      fputs("...\n", stderr);
    }

    prev_lineno = current_lineno;

    size_t line_length = p_diag_print_source_line(p_file, current_lineno);
    print_line_margin(0);

    uint32_t current_colno = 1;
    while (partial_source_ranges[i].lineno == current_lineno) {
      if (partial_source_ranges[i].colno_begin < current_colno)
        partial_source_ranges[i].colno_begin = current_colno;
      if (partial_source_ranges[i].colno_end == 0)
        partial_source_ranges[i].colno_end = line_length + 1;

      print_partial_src_range(&partial_source_ranges[i], current_colno);
      if (partial_source_ranges[i].colno_begin == partial_source_ranges[i].colno_end)
        current_colno = partial_source_ranges[i].colno_end + 1;
      else
        current_colno = partial_source_ranges[i].colno_end;

      ++i;
      if (i >= partial_source_range_count)
        break;
    }

    --i;
    fputs("\n", stderr);
  }
}

size_t
p_diag_print_source_line(PSourceFile* p_file, uint32_t p_lineno)
{
  assert(p_file != nullptr);

  uint32_t start_position = p_file->get_line_map().get_line_start_pos(p_lineno);

  size_t line_length = 0;
  auto it = std::next(p_file->get_buffer().cbegin(), start_position);
  while (*it != '\0' && *it != '\n' && *it != '\r') {
    ++line_length;
    ++it;
  }

  print_line_margin(p_lineno);
  fwrite(p_file->get_buffer_raw() + start_position, sizeof(char), line_length, stderr);
  fputs("\n", stderr);
  return line_length;
}
