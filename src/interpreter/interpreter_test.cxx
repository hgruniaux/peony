#include "../parser.hxx"
#include "interpreter.hxx"

#include <cmath>
#include <gtest/gtest.h>

class InterpreterTest : public ::testing::Test
{
public:
  PContext& ctx = PContext::get_global();
  PIdentifierTable identifier_table;
  PLexer lexer;
  std::unique_ptr<PParser> parser;
  std::unique_ptr<PSourceFile> source_file;

  void SetUp() override
  {
    p_identifier_table_init(&identifier_table);
    p_identifier_table_register_keywords(&identifier_table);
    lexer.identifier_table = &identifier_table;

    parser = std::make_unique<PParser>(ctx, lexer);
  }

  void TearDown() override { p_identifier_table_destroy(&identifier_table); }

  void check_expr(const char* p_input, const PInterpreterValue& p_expected)
  {
    set_test_input(p_input);
    PAstExpr* expr = parser->parse_standalone_expr();
    ASSERT_NE(expr, nullptr);

    PInterpreter interpreter(ctx);
    EXPECT_EQ(interpreter.eval(expr), p_expected);
  }

private:
  void set_test_input(const char* p_input)
  {
    source_file = std::make_unique<PSourceFile>("<test-input>", p_input);
    lexer.set_source_file(source_file.get());
  }
};

std::ostream&
operator<<(std::ostream& os, const PInterpreterValue& p_value)
{
  switch (p_value.get_kind()) {
    case PInterpreterValue::Kind::None:
      os << "None";
      break;
    case PInterpreterValue::Kind::Indeterminate:
      os << "Indeterminate";
      break;
    case PInterpreterValue::Kind::Bool:
      os << "Bool(" << ((p_value.get_bool()) ? "true" : "false") << ")";
      break;
    case PInterpreterValue::Kind::Integer:
      os << "Integer(" << p_value.get_integer() << ")";
      break;
    case PInterpreterValue::Kind::Float:
      os << "Float(" << p_value.get_float() << ")";
      break;
  }

  return os;
}

TEST_F(InterpreterTest, bool_literal)
{
  check_expr("true", PInterpreterValue::make_bool(true));
  check_expr("false", PInterpreterValue::make_bool(false));
}

TEST_F(InterpreterTest, int_literal)
{
  check_expr("58", PInterpreterValue::make_integer(58));
  check_expr("0b1011u8", PInterpreterValue::make_integer(11));
  check_expr("0x4ffffi64", PInterpreterValue::make_integer(327679));
  check_expr("0o456u64", PInterpreterValue::make_integer(302));
}

TEST_F(InterpreterTest, float_literal)
{
  check_expr("0.0", PInterpreterValue::make_float(0.0));
  check_expr("1.2f32", PInterpreterValue::make_float(1.2));
  check_expr("5.2f64", PInterpreterValue::make_float(5.2));
}

TEST_F(InterpreterTest, paren_expr)
{
  check_expr("(((((true)))))", PInterpreterValue::make_bool(true));
  check_expr("(5 + 2)", PInterpreterValue::make_integer(7));
}

TEST_F(InterpreterTest, unary_expr)
{
  check_expr("-true", PInterpreterValue::make_indeterminate());
  check_expr("-5", PInterpreterValue::make_integer(-5));
  check_expr("-5.0", PInterpreterValue::make_float(-5.0));

  check_expr("!true", PInterpreterValue::make_bool(false));
  check_expr("!false", PInterpreterValue::make_bool(true));
  check_expr("!5.0", PInterpreterValue::make_indeterminate());
}

TEST_F(InterpreterTest, binary_expr)
{
  check_expr("true + 5", PInterpreterValue::make_indeterminate());
  check_expr("42 + 3.14", PInterpreterValue::make_indeterminate());
  check_expr("2 + 8", PInterpreterValue::make_integer(10));
  check_expr("2.2 + 3.2", PInterpreterValue::make_float(5.4));

  check_expr("true - 5", PInterpreterValue::make_indeterminate());
  check_expr("42 - 3.14", PInterpreterValue::make_indeterminate());
  check_expr("2 - 8", PInterpreterValue::make_integer(-6));
  check_expr("2.2 - 3.2", PInterpreterValue::make_float(-1.0));

  check_expr("true * 5", PInterpreterValue::make_indeterminate());
  check_expr("42 * 3.14", PInterpreterValue::make_indeterminate());
  check_expr("2 * 8", PInterpreterValue::make_integer(16));
  check_expr("2.2 * 3.2", PInterpreterValue::make_float(2.2 * 3.2));

  check_expr("true / 5", PInterpreterValue::make_indeterminate());
  check_expr("42 / 3.14", PInterpreterValue::make_indeterminate());
  check_expr("8 / 4", PInterpreterValue::make_integer(2));
  check_expr("2.2 / 3.2", PInterpreterValue::make_float(2.2 / 3.2));

  check_expr("true % 5", PInterpreterValue::make_indeterminate());
  check_expr("42 % 3.14", PInterpreterValue::make_indeterminate());
  check_expr("9 % 4", PInterpreterValue::make_integer(1));
  check_expr("2.2 % 3.2", PInterpreterValue::make_float(std::fmod(2.2, 3.2)));
}

TEST_F(InterpreterTest, logical_and_or)
{
  check_expr("true && true", PInterpreterValue::make_bool(true));
  check_expr("true && false", PInterpreterValue::make_bool(false));
  check_expr("false && true", PInterpreterValue::make_bool(false));
  check_expr("false && false", PInterpreterValue::make_bool(false));

  check_expr("true || true", PInterpreterValue::make_bool(true));
  check_expr("true || false", PInterpreterValue::make_bool(true));
  check_expr("false || true", PInterpreterValue::make_bool(true));
  check_expr("false || false", PInterpreterValue::make_bool(false));

  // '1 + 2.0' is invalid and therefore indeterminate, but because || is a lazy operator this
  // should have no consequence. However, obviously the semantic analyzer will raise an error.
  check_expr("true || (1 + 2.0)", PInterpreterValue::make_bool(true));
  // Same for '&&'
  check_expr("false && (1 + 2.0)", PInterpreterValue::make_bool(false));
}

TEST_F(InterpreterTest, cast_expr)
{
  check_expr("false as i32", PInterpreterValue::make_integer(0));
  check_expr("true as i32", PInterpreterValue::make_integer(1));
  check_expr("false as f32", PInterpreterValue::make_float(0.0));
  check_expr("true as f32", PInterpreterValue::make_float(1.0));

  check_expr("5 as i32", PInterpreterValue::make_integer(5));
  check_expr("5 as i64", PInterpreterValue::make_integer(5));
  check_expr("0 as bool", PInterpreterValue::make_indeterminate());
  check_expr("5 as bool", PInterpreterValue::make_indeterminate());
  check_expr("5 as f32", PInterpreterValue::make_float(5.0));

  check_expr("5.0 as f32", PInterpreterValue::make_float(5.0));
  check_expr("5.0 as f64", PInterpreterValue::make_float(5.0));
  check_expr("0.0 as bool", PInterpreterValue::make_indeterminate());
  check_expr("5.0 as bool", PInterpreterValue::make_indeterminate());
  check_expr("2.0 as i32", PInterpreterValue::make_integer(2));
  check_expr("-2.0 as i32", PInterpreterValue::make_integer(-2));
}
