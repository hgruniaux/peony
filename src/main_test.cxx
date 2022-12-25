#include <gtest/gtest.h>

#include "type.hxx"
#include "utils/bump_allocator.hxx"

int
main(int p_argc, char** p_argv)
{
  ::testing::InitGoogleTest(&p_argc, p_argv);
  int result = RUN_ALL_TESTS();

  return result;
}
