#ifndef QUICKMACHO_H
#define QUICKMACHO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct dylib_info
{
  char *path;
};

struct mach_o_analysis
{
  char *architecture;
  struct dylib_info *dylibs;
  size_t num_dylibs;
};

void create_mach_o_analysis(struct mach_o_analysis *analysis);

void clean_mach_o_analysis(struct mach_o_analysis *analysis);

// Parse a Mach-O binary buffer
void parse_mach_o(uint8_t *buffer);

// Parse a Fat binary buffer
void parse_fat(uint8_t *buffer, size_t size);

// Check if a buffer is a Fat binary
bool is_fat_header(uint8_t *buffer);

#endif
