#include "quickmacho.h"

#include <stdio.h>
#include <stdlib.h>

void print_usage(const char *program_name)
{
  printf("Usage: %s <path-to-binary>\n", program_name);
  printf("Displays linked libraries in a Mach-O binary file\n");
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    print_usage(argv[0]);
    return 1;
  }

  const char *filename = argv[1];

  // Read the binary file
  FILE *file = fopen(filename, "rb");
  if (!file)
  {
    printf("Error: Cannot open file '%s'\n", filename);
    return 1;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Read file into buffer
  uint8_t *buffer = malloc(size);
  if (!buffer)
  {
    printf("Error: Memory allocation failed\n");
    fclose(file);
    return 1;
  }

  if (fread(buffer, 1, size, file) != size)
  {
    printf("Error: Failed to read file\n");
    free(buffer);
    fclose(file);
    return 1;
  }
  fclose(file);

  struct analysis analysis;
  create_analysis(&analysis);

  parse_macho(&analysis, buffer, size);

  if (analysis.is_fat)
  {
    printf("it is a fat binary\n");
    for (size_t i = 0; i < analysis.num_arch_analyses; i++)
    {
      printf("Architecture: %s\n", analysis.arch_analyses[i].architecture);
    }
  }
  else
  {
    printf("it is a regular Mach-O binary\n");
    printf("Architecture: %s\n", analysis.arch_analyses[0].architecture);
  }

  free(buffer);
  clean_analysis(&analysis);
  return 0;
}