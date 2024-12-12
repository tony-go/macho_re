extern "C" {
#include "../lib/libmachore.h"
}

#include <gtest/gtest.h>

#include <stdio.h>
#include <stdlib.h>

static void read_file_to_buffer(const char *filename, uint8_t **buffer,
                                size_t *size) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    printf("Error: Cannot open file '%s'\n", filename);
    return;
  }

  fseek(file, 0, SEEK_END);
  *size = ftell(file);
  fseek(file, 0, SEEK_SET);

  *buffer = (uint8_t *)malloc(*size);
  if (!*buffer) {
    printf("Error: Memory allocation failed\n");
    fclose(file);
    return;
  }

  if (fread(*buffer, 1, *size, file) != *size) {
    printf("Error: Failed to read file\n");
    free(*buffer);
    fclose(file);
    return;
  }
  fclose(file);
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

TEST(libmachore, parse_macho) {
  struct analysis analysis;
  create_analysis(&analysis);

  const char *filename = "/bin/ls";
  uint8_t *buffer = nullptr;
  size_t buffer_size = 0;
  read_file_to_buffer(filename, &buffer, &buffer_size);

  parse_macho(&analysis, buffer, buffer_size);

  EXPECT_EQ(analysis.num_arch_analyses, 2);
  EXPECT_EQ(analysis.is_fat, true);

  free(buffer);
  clean_analysis(&analysis);
}

TEST(libmachore, parse_macho_arch) {
  struct analysis analysis;
  create_analysis(&analysis);

  const char *filename = "/bin/ls";
  uint8_t *buffer = nullptr;
  size_t buffer_size = 0;
  read_file_to_buffer(filename, &buffer, &buffer_size);

  parse_macho(&analysis, buffer, buffer_size);
  struct arch_analysis *arch_analysis = &analysis.arch_analyses[0];

  EXPECT_STREQ(arch_analysis->architecture, "x86_64");
  EXPECT_EQ(arch_analysis->filetype, FILETYPE_EXEC);
  EXPECT_EQ(arch_analysis->num_dylibs, 3);

  free(buffer);
  clean_analysis(&analysis);
}
