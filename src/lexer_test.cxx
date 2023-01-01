#include "lexer.hxx"

#include <gtest/gtest.h>

class lexer_test : public ::testing::Test
{
protected:
  PIdentifierTable identifier_table;
  PLexer lexer;
  std::unique_ptr<PSourceFile> source_file;

  void SetUp() override
  {
    p_identifier_table_init(&identifier_table);
    p_identifier_table_register_keywords(&identifier_table);
    lexer.identifier_table = &identifier_table;
  }

  void TearDown() override
  {
    p_identifier_table_destroy(&identifier_table);
  }

  void set_input(const char* p_input)
  {
    source_file = std::make_unique<PSourceFile>("<test-input>", p_input);
    lexer.set_source_file(source_file.get());
  }

  PToken check_token(PTokenKind p_kind,
                     uint32_t p_expected_begin_line,
                     uint32_t p_expected_begin_col,
                     uint32_t p_expected_end_line,
                     uint32_t p_expected_end_col)
  {
    PToken token;
    lexer.tokenize(token);
    EXPECT_EQ(token.kind, p_kind);

    PSourceLocation begin_loc = token.source_location;
    PSourceLocation end_loc = begin_loc + token.token_length;

    uint32_t begin_line, begin_col;
    uint32_t end_line, end_col;
    p_source_location_get_lineno_and_colno(source_file.get(), begin_loc, &begin_line, &begin_col);
    p_source_location_get_lineno_and_colno(source_file.get(), end_loc, &end_line, &end_col);

    EXPECT_EQ(begin_line, p_expected_begin_line);
    EXPECT_EQ(begin_col, p_expected_begin_col);
    EXPECT_EQ(end_line, p_expected_end_line);
    EXPECT_EQ(end_col, p_expected_end_col);

    return token;
  }

  void check_int_literal(uint32_t p_begin_line,
                         uint32_t p_begin_col,
                         uint32_t p_end_line,
                         uint32_t p_end_col,
                         int p_radix,
                         PIntLiteralSuffix p_suffix)
  {
    PToken token = check_token(P_TOK_INT_LITERAL, p_begin_line, p_begin_col, p_end_line, p_end_col);
    EXPECT_EQ(token.data.literal.int_radix, p_radix);
    EXPECT_EQ(token.data.literal.suffix_kind, p_suffix);
  }
};

TEST_F(lexer_test, ident_and_keywords)
{
  set_input("foo r#foo i32 r#i32");

  PToken foo = check_token(P_TOK_IDENTIFIER, 1, 1, 1, 4);
  EXPECT_STREQ(foo.data.identifier->spelling, "foo");

  PToken raw_foo = check_token(P_TOK_IDENTIFIER, 1, 5, 1, 10);
  EXPECT_STREQ(raw_foo.data.identifier->spelling, "foo");

  PToken i32 = check_token(P_TOK_KEY_i32, 1, 11, 1, 14);
  EXPECT_STREQ(i32.data.identifier->spelling, "i32");

  PToken raw_i32 = check_token(P_TOK_IDENTIFIER, 1, 15, 1, 20);
  EXPECT_STREQ(raw_i32.data.identifier->spelling, "i32");

  check_token(P_TOK_EOF, 1, 20, 1, 20);
}

TEST_F(lexer_test, comments)
{
  // Single line comment with lexer.keep_comments = false
  set_input("foo // foo\nbar");
  lexer.set_keep_comments(false);
  check_token(P_TOK_IDENTIFIER, 1, 1, 1, 4);
  check_token(P_TOK_IDENTIFIER, 2, 1, 2, 4);
  check_token(P_TOK_EOF, 2, 4, 2, 4);

  // Single line comment with lexer.keep_comments = true
  set_input("foo // foo\nbar");
  lexer.set_keep_comments(true);
  check_token(P_TOK_IDENTIFIER, 1, 1, 1, 4);
  check_token(P_TOK_COMMENT, 1, 5, 1, 11);
  check_token(P_TOK_IDENTIFIER, 2, 1, 2, 4);
  check_token(P_TOK_EOF, 2, 4, 2, 4);

  // Block comment with lexer.keep_comments = false
  set_input("foo /* foo\nbar\nfoo */ bar");
  lexer.set_keep_comments(false);
  check_token(P_TOK_IDENTIFIER, 1, 1, 1, 4);
  check_token(P_TOK_IDENTIFIER, 3, 8, 3, 11);
  check_token(P_TOK_EOF, 3, 11, 3, 11);

  // Block comment with lexer.keep_comments = true
  set_input("foo /* foo\nbar\nfoo */ bar");
  lexer.set_keep_comments(true);
  check_token(P_TOK_IDENTIFIER, 1, 1, 1, 4);
  check_token(P_TOK_COMMENT, 1, 5, 3, 7);
  check_token(P_TOK_IDENTIFIER, 3, 8, 3, 11);
  check_token(P_TOK_EOF, 3, 11, 3, 11);
}

TEST_F(lexer_test, integer_literals)
{
  // Decimal integer literal
  set_input("0 10i32 5_2_8_ 5___5u64 8___i8");
  check_int_literal(1, 1, 1, 2, 10, P_ILS_NO_SUFFIX);
  check_int_literal(1, 3, 1, 8, 10, P_ILS_I32);
  check_int_literal(1, 9, 1, 15, 10, P_ILS_NO_SUFFIX);
  check_int_literal(1, 16, 1, 24, 10, P_ILS_U64);
  check_int_literal(1, 25, 1, 31, 10, P_ILS_I8);
  check_token(P_TOK_EOF, 1, 31, 1, 31);

  // Binary integer literal
  set_input("0b0 0B1i32 0b1_10_1__u8 0B__1_1___");
  check_int_literal(1, 1, 1, 4, 2, P_ILS_NO_SUFFIX);
  check_int_literal(1, 5, 1, 11, 2, P_ILS_I32);
  check_int_literal(1, 12, 1, 24, 2, P_ILS_U8);
  check_int_literal(1, 25, 1, 35, 2, P_ILS_NO_SUFFIX);
  check_token(P_TOK_EOF, 1, 35, 1, 35);

  // Hexadecimal integer literal
  set_input("0x15 0Xfai64 0x_7__ 0x5_f5_u16");
  check_int_literal(1, 1, 1, 5, 16, P_ILS_NO_SUFFIX);
  check_int_literal(1, 6, 1, 13, 16, P_ILS_I64);
  check_int_literal(1, 14, 1, 20, 16, P_ILS_NO_SUFFIX);
  check_int_literal(1, 21, 1, 31, 16, P_ILS_U16);
  check_token(P_TOK_EOF, 1, 31, 1, 31);
}

TEST_F(lexer_test, string_literals)
{
    set_input("\"ab\"");

    check_token(P_TOK_STRING_LITERAL, 1, 1, 1, 5);
}

TEST_F(lexer_test, operators)
{
  set_input("(){}[].,;:-> >>=");

  uint32_t pos = 1;
  check_token(P_TOK_LPAREN, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_RPAREN, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_LBRACE, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_RBRACE, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_LSQUARE, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_RSQUARE, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_DOT, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_COMMA, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_SEMI, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_COLON, 1, pos, 1, pos + 1);
  pos++;
  check_token(P_TOK_ARROW, 1, pos, 1, pos + 2);
  pos += 3;
  check_token(P_TOK_GREATER_GREATER_EQUAL, 1, pos, 1, pos + 3);
}
