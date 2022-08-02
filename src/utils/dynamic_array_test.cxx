#include "dynamic_array.h"

#include <gtest/gtest.h>

TEST(dynamic_array, append)
{
  PDynamicArray array;
  DYN_ARRAY_INIT(int, &array);
  ASSERT_EQ(array.size, 0);

  const char* item = "foo";
  DYN_ARRAY_APPEND(const char*, &array, item);
  EXPECT_EQ(array.size, 1);
  EXPECT_GE(array.capacity, 1);
  ASSERT_NE(array.buffer, nullptr);
  EXPECT_EQ(DYN_ARRAY_AT(const char*, &array, 0), item);

  DYN_ARRAY_DESTROY(int, &array);
}

TEST(dynamic_array, many_appends)
{
  PDynamicArray array;
  DYN_ARRAY_INIT(int, &array);

  for (int i = 0; i < 100; ++i) {
    DYN_ARRAY_APPEND(int, &array, i);
    EXPECT_EQ(array.size, i + 1);
    EXPECT_GE(array.capacity, i + 1);
    ASSERT_NE(array.buffer, nullptr);
  }

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(DYN_ARRAY_AT(int, &array, i), i);
  }

  DYN_ARRAY_DESTROY(int, &array);
}
