#include "quickmacho.h"

#include <stdio.h>
#include <stdlib.h>

void print_usage(const char *program_name)
{
  printf("Usage: %s <path-to-binary>\n", program_name);
  printf("Displays linked libraries in a Mach-O binary file\n");
}

void print_libraries(const struct arch_analysis *arch_analysis)
{
  printf("🔧 Architecture: %s\n", arch_analysis->architecture);
  printf("   ├─ Linked Libraries:\n");
  struct dylib_info *dylib_info = arch_analysis->dylibs;
  for (size_t dylib_index = 0; dylib_index < arch_analysis->num_dylibs; dylib_index++)
  {
    printf("   │  • %s\n", dylib_info[dylib_index].path);
    printf("   │   └─ Version: %s\n", dylib_info[dylib_index].version);
  }
  printf("   └────────────────\n");
}

void pretty_print_macho(const struct analysis *analysis)
{
  if (analysis->is_fat)
  {
    printf("📦 Fat Binary (Universal)\n");
    printf("═══════════════════════\n\n");

    for (size_t arch_index = 0; arch_index < analysis->num_arch_analyses; arch_index++)
    {
      print_libraries(&analysis->arch_analyses[arch_index]);
      printf("\n");
    }
  }
  else
  {
    printf("📱 Mach-O Binary\n");
    printf("══════════════\n");
    print_libraries(&analysis->arch_analyses[0]);
  }
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
  pretty_print_macho(&analysis);

  free(buffer);
  clean_analysis(&analysis);
  return 0;
}