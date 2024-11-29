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
    printf("📦 Fat Binary (Universal)\n");
    printf("═══════════════════════\n\n");

    for (size_t arch_index = 0; arch_index < analysis.num_arch_analyses; arch_index++)
    {
      printf("🔧 Architecture: %s\n", analysis.arch_analyses[arch_index].architecture);
      printf("   ├─ Linked Libraries:\n");
      struct dylib_info *dylib_info = analysis.arch_analyses[arch_index].dylibs;
      for (size_t dylib_index = 0; dylib_index < analysis.arch_analyses[arch_index].num_dylibs; dylib_index++)
      {
        printf("   │  • %s\n", dylib_info[dylib_index].path);
      }
      printf("   └────────────────\n\n");
    }
  }
  else
  {
    printf("📱 Mach-O Binary\n");
    printf("══════════════\n");
    printf("🔧 Architecture: %s\n", analysis.arch_analyses[0].architecture);
    printf("   ├─ Linked Libraries:\n");
    struct dylib_info *dylib_info = analysis.arch_analyses[0].dylibs;
    for (size_t dylib_index = 0; dylib_index < analysis.arch_analyses[0].num_dylibs; dylib_index++)
    {
      printf("   │  • %s\n", dylib_info[dylib_index].path);
    }
    printf("   └────────────────\n");
  }

  free(buffer);
  clean_analysis(&analysis);
  return 0;
}