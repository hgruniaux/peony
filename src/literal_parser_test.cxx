#include "literal_parser.hxx"

#include <gtest/gtest.h>

TEST(literal_parser, dec_int_literal)
{
  auto check_overflow = [](const char* p_input) {
    uintmax_t value;
    EXPECT_TRUE(parse_int_literal_token(p_input, p_input + strlen(p_input), 10, value));
  };

  auto check = [](const char* p_input, uintmax_t p_expected) {
    uintmax_t value;
    EXPECT_FALSE(parse_int_literal_token(p_input, p_input + strlen(p_input), 10, value));
    EXPECT_EQ(value, p_expected);
  };

  check("0", 0);
  check("42", 42);
  check("1_000_000", 1000000);
  check_overflow("9999999999999999999999999999");
}

TEST(literal_parser, bin_int_literal)
{
  auto check_overflow = [](const char* p_input) {
    uintmax_t value;
    EXPECT_TRUE(parse_int_literal_token(p_input, p_input + strlen(p_input), 2, value));
  };

  auto check = [](const char* p_input, uintmax_t p_expected) {
    uintmax_t value;
    EXPECT_FALSE(parse_int_literal_token(p_input, p_input + strlen(p_input), 2, value));
    EXPECT_EQ(value, p_expected);
  };

  check("0b0", 0);
  check("0B101", 5);
  check("0b1010_1100", 172);
  check("0b1111_1111_1001_0000", 65424);
  check("0b________1", 1);
  check_overflow("0b1111111111111111111111111111111111111111111111111111111111111111111111111");
}

TEST(literal_parser, oct_int_literal)
{
  auto check_overflow = [](const char* p_input) {
    uintmax_t value;
    EXPECT_TRUE(parse_int_literal_token(p_input, p_input + strlen(p_input), 8, value));
  };

  auto check = [](const char* p_input, uintmax_t p_expected) {
    uintmax_t value;
    EXPECT_FALSE(parse_int_literal_token(p_input, p_input + strlen(p_input), 8, value));
    EXPECT_EQ(value, p_expected);
  };

  check("0o0", 0);
  check("0O720", 464);
  check("0O132_7523", 372563);
  check_overflow("0O7777777777777777777777");
}

TEST(literal_parser, hex_int_literal)
{
  auto check_overflow = [](const char* p_input) {
    uintmax_t value;
    EXPECT_TRUE(parse_int_literal_token(p_input, p_input + strlen(p_input), 16, value));
  };

  auto check = [](const char* p_input, uintmax_t p_expected) {
    uintmax_t value;
    EXPECT_FALSE(parse_int_literal_token(p_input, p_input + strlen(p_input), 16, value));
    EXPECT_EQ(value, p_expected);
  };

  check("0x10", 16);
  check("0XAf", 175);
  check("0x5A_2f_3D", 5910333);
  check_overflow("0xffffffffffffffffffffff");
}

TEST(literal_parser, float_literal)
{
  auto does_overflow = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    double value;
    return parse_float_literal_token(p_input, end, value);
  };

  auto get_value = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    double value;
    EXPECT_FALSE(parse_float_literal_token(p_input, end, value));
    return value;
  };

  EXPECT_EQ(get_value("0.0"), 0.0);
  EXPECT_EQ(get_value("19.254"), 19.254);
  EXPECT_EQ(get_value("1.18973e+32"), 1.18973e+32);
  EXPECT_TRUE(does_overflow("1.18973e+4932"));
}

TEST(literal_parser, string_literal)
{
  auto check = [](const char* p_input, const std::string& p_expected) {
    EXPECT_EQ(parse_string_literal_token(p_input, p_input + strlen(p_input)), p_expected);
  };

  check(R"("Foo bar")", "Foo bar");
  check(R"("\"\'\n\r\t\\")", "\"\'\n\r\t\\");
  check(R"("\0")", std::string("\0", 1));
  check(R"("\x7f\x2A")", "\x7f\x2A");
  check(R"("\u{1}\u{4f}\u{7a2}\u{aBCd}\u{10ffff}")", "\x01\x4f\xde\xa2\xea\xaf\x8d\xf4\x8f\xbf\xbf");
}
