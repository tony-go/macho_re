#ifndef LIBMACHORE_H
#define LIBMACHORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LIBMACHORE_ARCHITECTURE_SIZE 16
#define LIBMACHORE_DYLIB_VERSION_SIZE 16
#define LIBMACHORE_DYLIB_PATH_SIZE 256
#define LIBMACHORE_ORIGINAL_SECTION_SIZE 24
#define LIBMACHORE_ORIGINAL_SEGMENT_SIZE 24
#define LIBMACHORE_SYMBOL_TYPE_SIZE 24

struct dylib_info {
  char path[LIBMACHORE_DYLIB_PATH_SIZE];
  bool is_path_truncated;
  char version[LIBMACHORE_DYLIB_VERSION_SIZE];
};

typedef enum {
  LIBMACHORE_FILETYPE_OBJECT,
  LIBMACHORE_FILETYPE_EXECUTE,
  LIBMACHORE_FILETYPE_FVMLIB,
  LIBMACHORE_FILETYPE_CORE,
  LIBMACHORE_FILETYPE_PRELOAD,
  LIBMACHORE_FILETYPE_DYLIB,
  LIBMACHORE_FILETYPE_DYLINKER,
  LIBMACHORE_FILETYPE_BUNDLE,
  LIBMACHORE_FILETYPE_DYLIB_STUB,
  LIBMACHORE_FILETYPE_DSYM,
  LIBMACHORE_FILETYPE_KEXT_BUNDLE,
  LIBMACHORE_FILETYPE_NOT_SUPPORTED,
} filetype_t;

struct string_info {
  char *content;
  size_t size;
  uint64_t original_offset;
  char original_section[LIBMACHORE_ORIGINAL_SECTION_SIZE];
  char original_segment[LIBMACHORE_ORIGINAL_SEGMENT_SIZE];
};

struct symbol_info {
  char *name;
  char type[LIBMACHORE_SYMBOL_TYPE_SIZE];
  bool has_no_section;
};

struct security_flags {
  bool is_signed;
  bool is_library_validation_disabled;
  bool is_dylib_env_var_allowed;
  bool has_hardened_runtime;
};

struct arch_analysis {
  char architecture[LIBMACHORE_ARCHITECTURE_SIZE];
  filetype_t filetype;

  // Flags
  bool no_undefined_refs;
  bool dyld_compatible;
  bool defines_weak_symbols;
  bool uses_weak_symbols;
  bool allows_stack_execution;
  bool enforce_no_heap_exec;

  // Dylibs
  struct dylib_info *dylibs;
  size_t num_dylibs;

  // Strings
  struct string_info *strings;
  size_t num_strings;

  // Symnols
  struct symbol_info *symbols;
  size_t num_symbols;

  // Codesign info
  struct security_flags *security_flags;
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
