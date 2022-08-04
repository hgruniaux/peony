#include "string.h"

#include <gtest/gtest.h>

TEST(string, init)
{
  PString string;
  string_init(&string);

  EXPECT_EQ(string.size, 0);
  EXPECT_STREQ(string.buffer, "");

  string_destroy(&string);
}

TEST(string, append)
{
  PString string;
  string_init(&string);
  
  string_append(&string, "Hello");
  EXPECT_EQ(string.size, 5);
  EXPECT_STREQ(string.buffer, "Hello");

  string_append(&string, " World");
  EXPECT_EQ(string.size, 11);
  EXPECT_STREQ(string.buffer, "Hello World");

  string_destroy(&string);
}
