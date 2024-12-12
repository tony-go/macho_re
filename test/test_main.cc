#include <gtest/gtest.h>

extern "C" {
#include "../libmachore.h"
}

TEST(libmachore, create_analysis) {
  struct analysis analysis;
  create_analysis(&analysis);

  EXPECT_EQ(analysis.arch_analyses, nullptr);
  EXPECT_EQ(analysis.num_arch_analyses, 0);
  EXPECT_EQ(analysis.is_fat, false);

  clean_analysis(&analysis);
}
