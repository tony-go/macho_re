#ifndef QUICKMACHO_H
#define QUICKMACHO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define QUICKMACHO_ARCHITECTURE_SIZE 16

struct dylib_info
{
  char *path;
};

struct arch_analysis
{
  char architecture[QUICKMACHO_ARCHITECTURE_SIZE];
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

// Parse a Mach-O binary buffer
void parse_mach_o(struct analysis *analysis, uint8_t *buffer);

// Parse a Fat binary buffer
void parse_fat(struct analysis *analysis, uint8_t *buffer, size_t size);

// Check if a buffer is a Fat binary
bool is_fat_header(uint8_t *buffer);

#endif
