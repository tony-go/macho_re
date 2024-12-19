#include "lib/libmachore.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *program_name) {
  printf("Usage: %s <path-to-binary>\n", program_name);
  printf("Displays linked libraries in a Mach-O binary file\n");
}

const char *filetype_to_string(filetype_t filetype) {
  switch (filetype) {
  case LIBMACHORE_FILETYPE_EXECUTE:
    return "Executable";
  case LIBMACHORE_FILETYPE_DYLIB:
    return "Dynamic Library";
  case LIBMACHORE_FILETYPE_BUNDLE:
    return "Bundle";
  case LIBMACHORE_FILETYPE_OBJECT:
    return "Object";
  case LIBMACHORE_FILETYPE_CORE:
    return "Core Dump";
  case LIBMACHORE_FILETYPE_NOT_SUPPORTED:
    return "Not Supported";
  default:
    return "Not Supported";
  }
}

void print_arch(const struct arch_analysis *arch_analysis) {
  printf("🔧 Architecture: %s\n", arch_analysis->architecture);
  printf("📁 File Type: %s\n", filetype_to_string(arch_analysis->filetype));

  printf("   ├─ Linked Libraries:\n");
  struct dylib_info *dylib_info = arch_analysis->dylibs;
  for (size_t dylib_index = 0; dylib_index < arch_analysis->num_dylibs;
       dylib_index++) {
    printf("   │  • %s\n", dylib_info[dylib_index].path);
    printf("   │   └─ Version: %s\n", dylib_info[dylib_index].version);
  }

  printf("   ├─ String:\n");
  struct string_info *string_info = arch_analysis->strings;
  for (size_t string_index = 0; string_index < arch_analysis->num_strings;
       string_index++) {
    const char *content = string_info[string_index].content;
    size_t length = string_info[string_index].size;

    printf("   │  • ");

    for (size_t i = 0; i < length; i++) {
      char c = content[i];
      switch (c) {
      case '\n':
        printf("\\n");
        break;
      case '\0':
        // We don't want to print the null terminator
        break;
      default:
        if (isprint(c)) {
          printf("%c", c);
        } else {
          printf("\\x%02x", (unsigned char)c);
        }
      }
    }

    printf(" \033[90m(%s,%s)\033[0m",
           string_info[string_index].original_segment,
           string_info[string_index].original_section);

    printf("\n");
  }
  printf("   └────────────────\n");
}

void pretty_print_macho(const struct analysis *analysis, const char *path,
                        bool is_first_only) {
  if (analysis->is_fat && !is_first_only) {
    printf("📦 Fat Binary\n");
    printf("📂 Path: %s\n", path);
    printf("═══════════════════════\n\n");

    for (size_t arch_index = 0; arch_index < analysis->num_arch_analyses;
         arch_index++) {
      print_arch(&analysis->arch_analyses[arch_index]);
      printf("\n");
    }
  } else {
    printf("📦 Mach-O Binary\n");
    printf("📂 Path: %s\n", path);
    printf("══════════════\n");
    print_arch(&analysis->arch_analyses[0]);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  const char *filename = argv[1];

  bool is_first_only = false;
  if (argc == 3) {
    const char *option = argv[2];
    if (strcmp(option, "--first-only") == 0) {
      is_first_only = true;
    }
  }

  // Read the binary file
  FILE *file = fopen(filename, "rb");
  if (!file) {
    printf("Error: Cannot open file '%s'\n", filename);
    return 1;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Read file into buffer
  uint8_t *buffer = malloc(size);
  if (!buffer) {
    printf("Error: Memory allocation failed\n");
    fclose(file);
    return 1;
  }

  if (fread(buffer, 1, size, file) != size) {
    printf("Error: Failed to read file\n");
    free(buffer);
    fclose(file);
    return 1;
  }
  fclose(file);

  struct analysis analysis;
  create_analysis(&analysis);

  parse_macho(&analysis, buffer, size);
  pretty_print_macho(&analysis, filename, is_first_only);

  free(buffer);
  clean_analysis(&analysis);
  return 0;
}
