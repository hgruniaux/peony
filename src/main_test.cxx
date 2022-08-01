#include <gtest/gtest.h>

#include "type.h"
#include "utils/bump_allocator.h"

int
main(int p_argc, char** p_argv)
{
  p_bump_init(&p_global_bump_allocator);
  p_init_types();

  ::testing::InitGoogleTest(&p_argc, p_argv);
  int result = RUN_ALL_TESTS();

  p_bump_destroy(&p_global_bump_allocator);
  return result;
}
