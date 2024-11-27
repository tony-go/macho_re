#ifndef QUICKMACHO_H
#define QUICKMACHO_H

#include <stdint.h>
#include <stddef.h>

// Parse a Mach-O binary buffer
void parse_mach_o(uint8_t *buffer);

// Parse a Fat binary buffer
void parse_fat(uint8_t *buffer, size_t size);
#endif