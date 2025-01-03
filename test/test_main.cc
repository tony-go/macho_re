extern "C" {
#include "../lib/libmachore.h"
}

#include <gtest/gtest.h>

#include <filesystem>
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

#define INIT_OUTPUT(filename)                                                  \
  struct machore_output_t output;                                              \
  init_output(&output);                                                        \
  uint8_t *buffer = nullptr;                                                   \
  size_t buffer_size = 0;                                                      \
  read_file_to_buffer(filename, &buffer, &buffer_size);

#define CLEAN_OUTPUT()                                                         \
  free(buffer);                                                                \
  clean_output(&output);

TEST(libmachore, create_analysis) {
  INIT_OUTPUT("/bin/ls");

  EXPECT_EQ(output.arch_analyses, nullptr);
  EXPECT_EQ(output.num_arch_analyses, 0);
  EXPECT_EQ(output.is_fat, false);

  CLEAN_OUTPUT();
}

TEST(libmachore, clean_analysis) {
  INIT_OUTPUT("/bin/ls");
  output.arch_analyses =
      (struct arch_analysis *)malloc(sizeof(struct arch_analysis));
  output.num_arch_analyses = 1;
  CLEAN_OUTPUT()

  EXPECT_EQ(output.arch_analyses, nullptr);
  EXPECT_EQ(output.num_arch_analyses, 0);
  EXPECT_EQ(output.is_fat, false);
}

TEST(libmachore, parse_macho_fat) {
  INIT_OUTPUT("/bin/ls");
  parse_macho(&output, buffer, buffer_size);

  EXPECT_EQ(output.num_arch_analyses, 2);
  EXPECT_EQ(output.is_fat, true);

  CLEAN_OUTPUT();
}

TEST(libmachore, parse_macho_arch) {
  INIT_OUTPUT("/bin/ls");

  parse_macho(&output, buffer, buffer_size);
  struct arch_analysis *arch_analysis = &output.arch_analyses[0];

  EXPECT_STREQ(arch_analysis->architecture, "x86_64");
  EXPECT_EQ(arch_analysis->filetype, LIBMACHORE_FILETYPE_EXECUTE);
  EXPECT_EQ(arch_analysis->num_dylibs, 3);
  // TODO: add string

  CLEAN_OUTPUT();
}

TEST(libmachore, parse_macho_filetypes) {
  struct machore_output_t output;
  uint8_t *buffer = nullptr;
  size_t buffer_size = 0;

  // Test MH_EXECUTE
  init_output(&output);
  read_file_to_buffer("/bin/ls", &buffer, &buffer_size);
  parse_macho(&output, buffer, buffer_size);
  EXPECT_EQ(output.arch_analyses[0].filetype, LIBMACHORE_FILETYPE_EXECUTE);
  free(buffer);
  clean_output(&output);

  // Test MH_DYLIB
  init_output(&output);
  read_file_to_buffer("/usr/lib/libgmalloc.dylib", &buffer, &buffer_size);
  parse_macho(&output, buffer, buffer_size);
  EXPECT_EQ(output.arch_analyses[0].filetype, LIBMACHORE_FILETYPE_DYLIB);
  free(buffer);
  clean_output(&output);

  // Test MH_BUNDLE
  init_output(&output);
  read_file_to_buffer(
      "/System/Library/CoreServices/SecurityAgentPlugins/DiskUnlock.bundle/"
      "Contents/MacOS/DiskUnlock",
      &buffer, &buffer_size);
  parse_macho(&output, buffer, buffer_size);
  EXPECT_EQ(output.arch_analyses[0].filetype, LIBMACHORE_FILETYPE_BUNDLE);
  free(buffer);
  clean_output(&output);

  // Test MH_DYLINKER
  init_output(&output);
  read_file_to_buffer("/usr/lib/dyld", &buffer, &buffer_size);
  parse_macho(&output, buffer, buffer_size);
  EXPECT_EQ(output.arch_analyses[0].filetype, LIBMACHORE_FILETYPE_DYLINKER);
  free(buffer);
  clean_output(&output);

  // Test MH_OBJECT
  auto object_binary_path =
      std::filesystem::current_path() / "fixtures" / "test.o";
  init_output(&output);
  read_file_to_buffer(object_binary_path.c_str(), &buffer, &buffer_size);
  parse_macho(&output, buffer, buffer_size);
  EXPECT_EQ(output.arch_analyses[0].filetype, LIBMACHORE_FILETYPE_OBJECT);
  free(buffer);
  clean_output(&output);

  // TEST MH_DSYM
  auto dsym_path = std::filesystem::current_path() / "fixtures" / "crash.dSYM" /
                   "Contents" / "Resources" / "DWARF" / "crash";
  init_output(&output);
  read_file_to_buffer(dsym_path.c_str(), &buffer, &buffer_size);
  parse_macho(&output, buffer, buffer_size);
  EXPECT_EQ(output.arch_analyses[0].filetype, LIBMACHORE_FILETYPE_DSYM);
  free(buffer);
  clean_output(&output);
}

TEST(libmachore, parse_macho_dylib) {
  INIT_OUTPUT("/bin/ls");
  parse_macho(&output, buffer, buffer_size);

  struct arch_analysis *arch_analysis = &output.arch_analyses[0];
  struct dylib_info *dylib_info_1 = &arch_analysis->dylibs[0];
  EXPECT_STREQ(dylib_info_1->path, "/usr/lib/libutil.dylib");
  EXPECT_FALSE(dylib_info_1->version[0] == '\0');

  struct dylib_info *dylib_info_2 = &arch_analysis->dylibs[1];
  EXPECT_STREQ(dylib_info_2->path, "/usr/lib/libncurses.5.4.dylib");
  EXPECT_FALSE(dylib_info_2->version[0] == '\0');

  struct dylib_info *dylib_info_3 = &arch_analysis->dylibs[2];
  EXPECT_STREQ(dylib_info_3->path, "/usr/lib/libSystem.B.dylib");
  EXPECT_FALSE(dylib_info_3->version[0] == '\0');

  CLEAN_OUTPUT();
}

TEST(libmachore, parse_macho_strings) {
  INIT_OUTPUT("/bin/ls");
  parse_macho(&output, buffer, buffer_size);

  struct arch_analysis *arch_analysis = &output.arch_analyses[0];
  struct string_info *string_info = &arch_analysis->strings[0];
  EXPECT_TRUE(string_info->content != NULL);
  EXPECT_EQ(string_info->size, strlen(string_info->content) + 1);
  EXPECT_STREQ(string_info->original_segment, "__TEXT");
  EXPECT_STREQ(string_info->original_section, "__const");
  EXPECT_TRUE(string_info->original_offset > 0);

  CLEAN_OUTPUT();
}

TEST(libmachore, parse_macho_flags) {
  INIT_OUTPUT("/bin/ls");
  parse_macho(&output, buffer, buffer_size);

  struct arch_analysis *arch_analysis = &output.arch_analyses[0];
  EXPECT_TRUE(arch_analysis->no_undefined_refs);
  EXPECT_TRUE(arch_analysis->dyld_compatible);
  EXPECT_FALSE(arch_analysis->defines_weak_symbols);
  EXPECT_FALSE(arch_analysis->uses_weak_symbols);
  EXPECT_FALSE(arch_analysis->allows_stack_execution);
  EXPECT_FALSE(arch_analysis->enforce_no_heap_exec);

  CLEAN_OUTPUT();
}

TEST(libmachore, parse_macho_entitelements) {
  INIT_OUTPUT("/bin/ls");
  parse_macho(&output, buffer, buffer_size);

  struct arch_analysis *arch_analysis = &output.arch_analyses[0];
  EXPECT_FALSE(arch_analysis->security_flags->is_library_validation_disabled);
  EXPECT_FALSE(arch_analysis->security_flags->is_library_validation_disabled);
  EXPECT_FALSE(arch_analysis->security_flags->is_dylib_env_var_allowed);
  EXPECT_TRUE(arch_analysis->security_flags->is_signed);

  CLEAN_OUTPUT();
}

TEST(libmachore, parse_macho_symbols) {
  INIT_OUTPUT("/bin/ls");
  parse_macho(&output, buffer, buffer_size);

  struct arch_analysis *arch_analysis = &output.arch_analyses[0];
  struct symbol_info *symbols = arch_analysis[0].symbols;
  EXPECT_STREQ(symbols[0].name, "radr://5614542");
  EXPECT_STREQ(symbols[0].type, "STAB");
  EXPECT_TRUE(symbols[0].has_no_section);
  EXPECT_STREQ(symbols[1].name, "__mh_execute_header");
  EXPECT_STREQ(symbols[1].type, "EXTERNAL");
  EXPECT_FALSE(symbols[1].has_no_section);
  EXPECT_STREQ(symbols[2].name, "__DefaultRuneLocale");
  EXPECT_STREQ(symbols[2].type, "EXTERNAL");
  EXPECT_TRUE(symbols[2].has_no_section);

  CLEAN_OUTPUT();
}
