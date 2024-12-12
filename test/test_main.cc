#include <gtest/gtest.h>

extern "C" {
#include "../lib/libmachore.h"
}

TEST(libmachore, create_analysis) {
  struct analysis analysis;
  create_analysis(&analysis);

  EXPECT_EQ(analysis.arch_analyses, nullptr);
  EXPECT_EQ(analysis.num_arch_analyses, 0);
  EXPECT_EQ(analysis.is_fat, false);

  clean_analysis(&analysis);
}

TEST(libmachore, clean_analysis) {
  struct analysis analysis;
  create_analysis(&analysis);
  analysis.arch_analyses =
      (struct arch_analysis *)malloc(sizeof(struct arch_analysis));
  analysis.num_arch_analyses = 1;

  clean_analysis(&analysis);

  EXPECT_EQ(analysis.arch_analyses, nullptr);
  EXPECT_EQ(analysis.num_arch_analyses, 0);
  EXPECT_EQ(analysis.is_fat, false);
}
