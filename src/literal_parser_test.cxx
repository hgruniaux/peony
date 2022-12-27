#include "literal_parser.hxx"

#include <gtest/gtest.h>

TEST(literal_parser, dec_int_literal)
{
  auto does_overflow = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    uintmax_t value;
    return parse_int_literal_token(p_input, end, 10, value);
  };

  auto get_value = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    uintmax_t value;
    EXPECT_FALSE(parse_int_literal_token(p_input, end, 10, value));
    return value;
  };

  EXPECT_EQ(get_value("0"), 0);
  EXPECT_EQ(get_value("42"), 42);
  EXPECT_EQ(get_value("1_000_000"), 1000000);
  EXPECT_TRUE(does_overflow("9999999999999999999999999999"));
}

TEST(literal_parser, bin_int_literal)
{
  auto does_overflow = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    uintmax_t value;
    return parse_int_literal_token(p_input, end, 2, value);
  };

  auto get_value = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    uintmax_t value;
    EXPECT_FALSE(parse_int_literal_token(p_input, end, 2, value));
    return value;
  };

  EXPECT_EQ(get_value("0b0"), 0);
  EXPECT_EQ(get_value("0B101"), 5);
  EXPECT_EQ(get_value("0b1010_1100"), 172);
  EXPECT_TRUE(does_overflow("0b1111111111111111111111111111111111111111111111111111111111111111111111111"));
}

TEST(literal_parser, oct_int_literal)
{
  auto does_overflow = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    uintmax_t value;
    return parse_int_literal_token(p_input, end, 8, value);
  };

  auto get_value = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    uintmax_t value;
    EXPECT_FALSE(parse_int_literal_token(p_input, end, 8, value));
    return value;
  };

  EXPECT_EQ(get_value("0o0"), 0);
  EXPECT_EQ(get_value("0O720"), 464);
  EXPECT_EQ(get_value("0O132_7523"), 372563);
  EXPECT_TRUE(does_overflow("0O7777777777777777777777"));
}

TEST(literal_parser, hex_int_literal)
{
  auto does_overflow = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    uintmax_t value;
    return parse_int_literal_token(p_input, end, 16, value);
  };

  auto get_value = [](const char* p_input) {
    const char* end = p_input + strlen(p_input);
    uintmax_t value;
    EXPECT_FALSE(parse_int_literal_token(p_input, end, 16, value));
    return value;
  };

  EXPECT_EQ(get_value("0x10"), 16);
  EXPECT_EQ(get_value("0XAf"), 175);
  EXPECT_EQ(get_value("0x5A_2f_3D"), 5910333);
  EXPECT_TRUE(does_overflow("0xffffffffffffffffffffff"));
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
