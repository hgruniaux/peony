#include "line_map.hxx"

#include <gtest/gtest.h>

TEST(line_map, empty)
{
  PLineMap lm;

  // Lines and columns are 1-numbered
  EXPECT_EQ(lm.get_lineno(0), 1);
  EXPECT_EQ(lm.get_colno(0), 1);

  // No line terminator was registered
  EXPECT_EQ(lm.get_lineno(15861), 1);
  EXPECT_EQ(lm.get_colno(15861), 15862);
}

TEST(line_map, add)
{
  PLineMap lm;

  lm.add(50);

  // Check the position before the new line
  EXPECT_EQ(lm.get_lineno(49), 1);
  EXPECT_EQ(lm.get_colno(49), 50);

  // Check the position of the new line
  EXPECT_EQ(lm.get_lineno(50), 2);
  EXPECT_EQ(lm.get_colno(50), 1);

  // Check the position after the new line
  EXPECT_EQ(lm.get_lineno(51), 2);
  EXPECT_EQ(lm.get_colno(1), 2);

  lm.add(100);

  // Check the position before the old line
  EXPECT_EQ(lm.get_lineno(49), 1);
  EXPECT_EQ(lm.get_colno(49), 50);

  // Check the position of the old line
  EXPECT_EQ(lm.get_lineno(50), 2);
  EXPECT_EQ(lm.get_colno(50), 1);

  // Check the position after the old line
  EXPECT_EQ(lm.get_lineno(51), 2);
  EXPECT_EQ(lm.get_colno(51), 2);
}

TEST(line_map, many_adds)
{
  PLineMap lm;

  for (int i = 0; i < 1000; ++i) {
    lm.add(i);
  }
}

TEST(line_map, get_line_start_position)
{
  PLineMap lm;

  lm.add(50);
  lm.add(100);

  EXPECT_EQ(lm.get_line_start_pos(1), 0);
  EXPECT_EQ(lm.get_line_start_pos(2), 50);
  EXPECT_EQ(lm.get_line_start_pos(3), 100);
}
