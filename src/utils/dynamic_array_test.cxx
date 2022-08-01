#include "dynamic_array.h"

#include <gtest/gtest.h>

TEST(dynamic_array, append)
{
  PDynamicArray array;
  p_dynamic_array_init(&array);
  ASSERT_EQ(array.size, 0);

  const char* item = "foo";
  p_dynamic_array_append(&array, (PDynamicArrayItem)item);
  EXPECT_EQ(array.size, 1);
  EXPECT_GE(array.capacity, 1);
  ASSERT_NE(array.buffer, nullptr);
  EXPECT_EQ(array.buffer[0], item);

  p_dynamic_array_destroy(&array);
}

TEST(dynamic_array, many_appends)
{
  PDynamicArray array;
  p_dynamic_array_init(&array);

  for (int i = 0; i < 100; ++i) {
    p_dynamic_array_append(&array, (PDynamicArrayItem)i);
    EXPECT_EQ(array.size, i + 1);
    EXPECT_GE(array.capacity, i + 1);
    ASSERT_NE(array.buffer, nullptr);
  }

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(array.buffer[i], (PDynamicArrayItem)i);
  }

  p_dynamic_array_destroy(&array);
}
