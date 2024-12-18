#ifndef LIBMACHORE_H
#define LIBMACHORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LIBMACHORE_ARCHITECTURE_SIZE 16
#define LIBMACHORE_DYLIB_VERSION_SIZE 16
#define LIBMACHORE_DYLIB_PATH_SIZE 256

struct dylib_info {
  char path[LIBMACHORE_DYLIB_PATH_SIZE];
  bool is_path_truncated;
  char version[LIBMACHORE_DYLIB_VERSION_SIZE];
};

typedef enum {
  FILETYPE_EXEC,
  FILETYPE_DYLIB,
  FILETYPE_BUNDLE,
  FILETYPE_OBJECT,
  FILETYPE_CORE_DUMP,
  FILETYPE_NOT_SUPPORTED
} filetype_t;

struct string_info {
  char *content;
  size_t size;
  uint64_t original_offset;
  char original_section[24];
};

struct arch_analysis {
  char architecture[LIBMACHORE_ARCHITECTURE_SIZE];
  filetype_t filetype;

  // Dylibs
  struct dylib_info *dylibs;
  size_t num_dylibs;

  // Strings
  struct string_info *strings;
  size_t num_strings;
};

struct analysis {
  struct arch_analysis *arch_analyses;
  size_t num_arch_analyses;
  bool is_fat;
};

void create_analysis(struct analysis *analysis);

void clean_analysis(struct analysis *analysis);

void parse_macho(struct analysis *analysis, uint8_t *buffer, size_t size);

#endif
