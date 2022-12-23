#include "line_map.hxx"

#include <gtest/gtest.h>

TEST(line_map, empty)
{
  PLineMap lm;
  p_line_map_init(&lm);

  // Lines and columns are 1-numbered
  EXPECT_EQ(p_line_map_get_lineno(&lm, 0), 1);
  EXPECT_EQ(p_line_map_get_colno(&lm, 0), 1);

  // No line terminator was registered
  EXPECT_EQ(p_line_map_get_lineno(&lm, 15861), 1);
  EXPECT_EQ(p_line_map_get_colno(&lm, 15861), 15862);

  p_line_map_destroy(&lm);
}

TEST(line_map, add)
{
  PLineMap lm;
  p_line_map_init(&lm);

  p_line_map_add(&lm, 50);

  // Check the position before the new line
  EXPECT_EQ(p_line_map_get_lineno(&lm, 49), 1);
  EXPECT_EQ(p_line_map_get_colno(&lm, 49), 50);

  // Check the position of the new line
  EXPECT_EQ(p_line_map_get_lineno(&lm, 50), 2);
  EXPECT_EQ(p_line_map_get_colno(&lm, 50), 1);

  // Check the position after the new line
  EXPECT_EQ(p_line_map_get_lineno(&lm, 51), 2);
  EXPECT_EQ(p_line_map_get_colno(&lm, 51), 2);

  p_line_map_add(&lm, 100);

  // Check the position before the old line
  EXPECT_EQ(p_line_map_get_lineno(&lm, 49), 1);
  EXPECT_EQ(p_line_map_get_colno(&lm, 49), 50);

  // Check the position of the old line
  EXPECT_EQ(p_line_map_get_lineno(&lm, 50), 2);
  EXPECT_EQ(p_line_map_get_colno(&lm, 50), 1);

  // Check the position after the old line
  EXPECT_EQ(p_line_map_get_lineno(&lm, 51), 2);
  EXPECT_EQ(p_line_map_get_colno(&lm, 51), 2);

  p_line_map_destroy(&lm);
}

TEST(line_map, many_adds)
{
  PLineMap lm;
  p_line_map_init(&lm);

  for (int i = 0; i < 1000; ++i) {
    p_line_map_add(&lm, i);
  }

  p_line_map_destroy(&lm);
}

TEST(line_map, get_line_start_position)
{
  PLineMap lm;
  p_line_map_init(&lm);

  p_line_map_add(&lm, 50);
  p_line_map_add(&lm, 100);

  EXPECT_EQ(p_line_map_get_line_start_position(&lm, 1), 0);
  EXPECT_EQ(p_line_map_get_line_start_position(&lm, 2), 50);
  EXPECT_EQ(p_line_map_get_line_start_position(&lm, 3), 100);

  p_line_map_destroy(&lm);
}
