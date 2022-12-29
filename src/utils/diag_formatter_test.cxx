#include "diag_formatter.hxx"

#include "../identifier_table.hxx"
#include "../options.hxx"
#include "../type.hxx"
#include "context.hxx"

#include <gtest/gtest.h>

static void
check_arg_format(PDiagArgument* p_arg, const char* p_expected)
{
  std::string buffer;
  p_diag_format_arg(buffer, p_arg);
  EXPECT_EQ(buffer, p_expected);
}

static void
check_format(const char* p_msg, PDiagArgument* p_args, size_t p_arg_count, const char* p_expected)
{
  std::string buffer;
  p_diag_format_msg(buffer, p_msg, p_args, p_arg_count);
  EXPECT_EQ(buffer, p_expected);
}

TEST(diag_formatter, char_arg)
{
  // Printable character
  PDiagArgument print_arg;
  print_arg.type = P_DAT_CHAR;
  print_arg.value_char = '=';
  check_arg_format(&print_arg, "=");

  // Not printable character
  PDiagArgument ctrl_arg;
  ctrl_arg.type = P_DAT_CHAR;
  ctrl_arg.value_char = '\x03';
  check_arg_format(&ctrl_arg, "\\x03");
}

TEST(diag_formatter, int_arg)
{
  PDiagArgument arg;
  arg.type = P_DAT_INT;
  arg.value_int = 42;
  check_arg_format(&arg, "42");
}

TEST(diag_formatter, str_arg)
{
  PDiagArgument arg;
  arg.type = P_DAT_STR;
  arg.value_str = "foo";
  check_arg_format(&arg, "foo");
}

TEST(diag_formatter, tok_kind_arg)
{
  // Token with spelling.
  PDiagArgument arg;
  arg.type = P_DAT_TOK_KIND;
  arg.value_token_kind = P_TOK_LPAREN;
  check_arg_format(&arg, "(");

  // Token without spelling.
  PDiagArgument arg1;
  arg1.type = P_DAT_TOK_KIND;
  arg1.value_token_kind = P_TOK_EOF;
  check_arg_format(&arg1, "EOF");
}

TEST(diag_formatter, ident_arg)
{
  PIdentifierTable table;
  p_identifier_table_init(&table);

  PDiagArgument arg;
  arg.type = P_DAT_IDENT;
  arg.value_ident = p_identifier_table_get(&table, "foo", nullptr);
  check_arg_format(&arg, "foo");

  p_identifier_table_destroy(&table);
}

TEST(diag_formatter, type_arg)
{
  auto& ctx = PContext::get_global();

  // Builtin type.
  PDiagArgument builtin_type;
  builtin_type.type = P_DAT_TYPE;
  builtin_type.value_type = ctx.get_i32_ty();
  check_arg_format(&builtin_type, "i32");

  // Pointer type.
  PDiagArgument pointer_type;
  pointer_type.type = P_DAT_TYPE;
  pointer_type.value_type = ctx.get_pointer_ty(ctx.get_i32_ty());
  check_arg_format(&pointer_type, "*i32");

  // Parenthesized type.
  PDiagArgument paren_type;
  paren_type.type = P_DAT_TYPE;
  paren_type.value_type = ctx.get_paren_ty(ctx.get_i32_ty());
  check_arg_format(&paren_type, "i32"); // do not display parentheses

  {
    // Function type.
    PType* func_args[] = { ctx.get_bool_ty(), ctx.get_f32_ty() };
    PDiagArgument func_type;
    func_type.type = P_DAT_TYPE;
    func_type.value_type = ctx.get_function_ty(ctx.get_i32_ty(), { func_args, std::size(func_args) });
    check_arg_format(&func_type, "fn (bool, f32) -> i32");
  }

  {
    // Function type with no return type.
    PType* func_args[] = { ctx.get_char_ty() };
    PDiagArgument func_type;
    func_type.type = P_DAT_TYPE;
    func_type.value_type = ctx.get_function_ty(ctx.get_void_ty(), { func_args, std::size(func_args) });
    check_arg_format(&func_type, "fn (char)");
  }

  {
    // Function type without parameters.
    PDiagArgument func_type;
    func_type.type = P_DAT_TYPE;
    func_type.value_type = ctx.get_function_ty(ctx.get_pointer_ty(ctx.get_u8_ty()), {});
    check_arg_format(&func_type, "fn () -> *u8");
  }

  // TODO: add test for array types
  // TODO: add test for all builtin types
  // TODO: add test for tag types
}

TEST(diag_formatter, plural_s)
{
  PDiagArgument two;
  two.type = P_DAT_INT;
  two.value_int = 2;

  PDiagArgument one;
  one.type = P_DAT_INT;
  one.value_int = 1;

  PDiagArgument zero;
  zero.type = P_DAT_INT;
  zero.value_int = 0;

  PDiagArgument args[] = { two, one, zero };
  check_format("%0s %1s %2s", args, std::size(args), "s  ");
}

TEST(diag_formatter, quote)
{
  const bool enable_ansi_color_save = g_options.opt_diagnostics_color;

  g_options.opt_diagnostics_color = true;
  check_format("<%foo%>", nullptr, 0, "\x1b[1m'foo'\x1b[0m");
  g_options.opt_diagnostics_color = false;
  check_format("<%foo%>", nullptr, 0, "'foo'");

  g_options.opt_diagnostics_color = enable_ansi_color_save;
}

TEST(diag_formatter, args)
{
  PDiagArgument arg1;
  arg1.type = P_DAT_INT;
  arg1.value_int = 42;

  PDiagArgument arg2;
  arg2.type = P_DAT_STR;
  arg2.value_str = "bar";

  PDiagArgument args[] = { arg1, arg2 };
  check_format("{1} {0} {1} {0}", args, std::size(args), "bar 42 bar 42");
}
