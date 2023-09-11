#include "identifier_table.hxx"

#include <gtest/gtest.h>

TEST(identifier_table, get_not_exists)
{
  PIdentifierTable table;

  PIdentifierInfo* ident1 = table.get("foo", nullptr);

  char spelling[] = { 'b', 'a', 'r' };
  PIdentifierInfo* ident2 = table.get(spelling, spelling + 3);

  ASSERT_NE(ident1, nullptr);
  EXPECT_EQ(ident1->get_token_kind(), P_TOK_IDENTIFIER);
  EXPECT_EQ(ident1->get_spelling(), "foo");

  ASSERT_NE(ident2, nullptr);
  EXPECT_EQ(ident2->get_token_kind(), P_TOK_IDENTIFIER);
  EXPECT_EQ(ident2->get_spelling(), "bar");
}

TEST(identifier_table, get_exists)
{
  PIdentifierTable table;

  PIdentifierInfo* ident = table.get("foo", nullptr);
  ASSERT_NE(ident, nullptr);

  char foo[] = { 'f', 'o', 'o' };
  PIdentifierInfo* ident_again = table.get(foo, foo + 3);
  ASSERT_NE(ident_again, nullptr);
  EXPECT_EQ(ident, ident_again);
}

TEST(identifier_table, many_gets)
{
  PIdentifierTable table;

  PIdentifierInfo* foo = table.get("foo", nullptr);

  for (int i = 0; i < 250; ++i) {
    auto spelling = std::to_string(i);
    PIdentifierInfo* ident = table.get(spelling.c_str(), nullptr);
    ASSERT_NE(ident, nullptr);
    EXPECT_EQ(ident->get_spelling(), spelling.c_str());
  }

  // Even in case of rehashing, pointers MUST NOT be invalidated.
  PIdentifierInfo* foo_again = table.get("foo", nullptr);
  EXPECT_EQ(foo, foo_again);
  EXPECT_EQ(foo->get_spelling(), "foo");
}

TEST(identifier_table, keywords)
{
  PIdentifierTable table;

  PIdentifierInfo* i32_key = table.get("i32", nullptr);
  EXPECT_EQ(i32_key->get_token_kind(), P_TOK_IDENTIFIER);

  table.register_keywords();
  EXPECT_EQ(i32_key->get_token_kind(), P_TOK_KEY_i32);

  PIdentifierInfo* true_key = table.get("true", nullptr);
  EXPECT_EQ(true_key->get_token_kind(), P_TOK_KEY_true);
}
