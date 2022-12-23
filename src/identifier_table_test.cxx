#include "identifier_table.hxx"

#include <gtest/gtest.h>

TEST(identifier_table, init)
{
  PIdentifierTable table;
  p_identifier_table_init(&table);

  EXPECT_EQ(table.item_count, 0);

  p_identifier_table_destroy(&table);
}

TEST(identifier_table, get_not_exists)
{
  PIdentifierTable table;
  p_identifier_table_init(&table);

  PIdentifierInfo* ident1 = p_identifier_table_get(&table, "foo", nullptr);

  char spelling[] = { 'b', 'a', 'r' };
  PIdentifierInfo* ident2 = p_identifier_table_get(&table, spelling, spelling + 3);

  ASSERT_NE(ident1, nullptr);
  EXPECT_EQ(ident1->token_kind, P_TOK_IDENTIFIER);
  EXPECT_EQ(ident1->spelling_len, 3);
  EXPECT_STREQ(ident1->spelling, "foo");

  ASSERT_NE(ident2, nullptr);
  EXPECT_EQ(ident2->token_kind, P_TOK_IDENTIFIER);
  EXPECT_EQ(ident2->spelling_len, 3);
  EXPECT_STREQ(ident2->spelling, "bar");

  p_identifier_table_destroy(&table);
}

TEST(identifier_table, get_exists)
{
  PIdentifierTable table;
  p_identifier_table_init(&table);

  PIdentifierInfo* ident = p_identifier_table_get(&table, "foo", nullptr);
  ASSERT_NE(ident, nullptr);

  char foo[] = { 'f', 'o', 'o' };
  PIdentifierInfo* ident_again = p_identifier_table_get(&table, foo, foo + 3);
  ASSERT_NE(ident_again, nullptr);
  EXPECT_EQ(ident, ident_again);

  p_identifier_table_destroy(&table);
}

TEST(identifier_table, many_gets)
{
  PIdentifierTable table;
  p_identifier_table_init(&table);

  PIdentifierInfo* foo = p_identifier_table_get(&table, "foo", nullptr);

  for (int i = 0; i < 250; ++i) {
    auto spelling = std::to_string(i);
    PIdentifierInfo* ident = p_identifier_table_get(&table, spelling.c_str(), nullptr);
    ASSERT_NE(ident, nullptr);
    EXPECT_STREQ(ident->spelling, spelling.c_str());
  }

  // Even in case of rehashing, pointers MUST NOT be invalidated.
  PIdentifierInfo* foo_again = p_identifier_table_get(&table, "foo", nullptr);
  EXPECT_EQ(foo, foo_again);
  EXPECT_STREQ(foo->spelling, "foo");

  p_identifier_table_destroy(&table);
}

TEST(identifier_table, keywords)
{
  PIdentifierTable table;
  p_identifier_table_init(&table);

  PIdentifierInfo* i32_key = p_identifier_table_get(&table, "i32", nullptr);
  EXPECT_EQ(i32_key->token_kind, P_TOK_IDENTIFIER);

  p_identifier_table_register_keywords(&table);
  EXPECT_EQ(i32_key->token_kind, P_TOK_KEY_i32);

  PIdentifierInfo* true_key = p_identifier_table_get(&table, "true", nullptr);
  EXPECT_EQ(true_key->token_kind, P_TOK_KEY_true);

  p_identifier_table_destroy(&table);
}
