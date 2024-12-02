#ifndef LIBMACHORE_H
#define LIBMACHORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LIBMACHORE_ARCHITECTURE_SIZE 16
#define LIBMACHORE_DYLIB_VERSION_SIZE 16
#define LIBMACHORE_DYLIB_PATH_SIZE 256

struct dylib_info
{
  char path[LIBMACHORE_DYLIB_PATH_SIZE];
  bool is_path_truncated;
  char version[LIBMACHORE_DYLIB_VERSION_SIZE];
  bool is_version_truncated;
};

struct arch_analysis
{
  char architecture[LIBMACHORE_ARCHITECTURE_SIZE];
  struct dylib_info *dylibs;
  size_t num_dylibs;
};

struct analysis
{
  struct arch_analysis *arch_analyses;
  size_t num_arch_analyses;
  bool is_fat;
};

void create_analysis(struct analysis *analysis);

void clean_analysis(struct analysis *analysis);

void parse_macho(struct analysis *analysis, uint8_t *buffer, size_t size);

#endif
