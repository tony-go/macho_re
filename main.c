#include "lib/libmachore.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  DISPLAY_STRINGS = 0x1,
  DISPLAY_SYMBOLS = 0x2,
};

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
  case LIBMACHORE_FILETYPE_DYLINKER:
    return "Dynamic Linker";
  case LIBMACHORE_FILETYPE_DSYM:
    return "Debug Symbols";
  case LIBMACHORE_FILETYPE_NOT_SUPPORTED:
    return "Not Supported";
  default:
    return "Not Supported";
  }
}

void print_arch(const struct machore_arch_output_t *arch_output,
                uint8_t display_flags) {
  printf("🔧 Architecture: %s\n", arch_output->architecture);
  printf("📁 File Type: %s\n", filetype_to_string(arch_output->filetype));

  printf("   ├─ Binary Flags:\n");
  printf("   │  • No Undefined References: %s\n",
         arch_output->no_undefined_refs ? "Yes" : "No");
  printf("   │  • Dyld Compatible: %s\n",
         arch_output->dyld_compatible ? "Yes" : "No");
  printf("   │  • Defines Weak Symbols: %s\n",
         arch_output->defines_weak_symbols ? "Yes" : "No");
  printf("   │  • Uses Weak Symbols: %s\n",
         arch_output->uses_weak_symbols ? "Yes" : "No");
  printf("   │  • Allows Stack Execution: %s\n",
         arch_output->allows_stack_execution ? "Yes" : "No");
  printf("   │  • Enforce No Heap Execution: %s\n",
         arch_output->enforce_no_heap_exec ? "Yes" : "No");

  printf("   ├─ Security Flags:\n");
  printf("   │  • Is Signed: %s\n",
         arch_output->security_flags->is_signed ? "Yes" : "No");
  printf("   │  • Library Validation Disabled: %s\n",
         arch_output->security_flags->is_library_validation_disabled ? "Yes"
                                                                     : "No");
  printf("   │  • Dylib Environment Variable allowed: %s \n",
         arch_output->security_flags->is_dylib_env_var_allowed ? "Yes" : "No");
  printf("   │  • Hardened Runtime: %s\n",
         arch_output->security_flags->has_hardened_runtime ? "Yes" : "No");

  printf("   ├─ Linked Libraries:\n");
  struct dylib_info *dylib_info = arch_output->dylibs;
  for (size_t dylib_index = 0; dylib_index < arch_output->num_dylibs;
       dylib_index++) {
    printf("   │  • %s\n", dylib_info[dylib_index].path);
    printf("   │   └─ Version: %s\n", dylib_info[dylib_index].version);
  }

  if (display_flags & DISPLAY_STRINGS) {
    printf("   ├─ String:\n");
    struct string_info *string_info = arch_output->strings;
    // NOTE: We only print the first 20 strings
    for (size_t string_index = 0; string_index < arch_output->num_strings;
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
    printf("   │  (%zu strings)\n", arch_output->num_strings);
    printf("   └────────────────\n");
  }

  if (display_flags & DISPLAY_SYMBOLS) {
    printf("   ├─ Symbols:\n");
    struct symbol_info *symbol_info = arch_output->symbols;
    // NOTE: We only print the first 20 strings
    size_t max_printed_symbols =
        arch_output->num_symbols < 20 ? arch_output->num_symbols : 20;
    for (size_t symbol_index = 0; symbol_index < max_printed_symbols;
         symbol_index++) {
      struct symbol_info *sym = &symbol_info[symbol_index];

      printf("   │  • %s \033[90m(%s)\033[0m \n", sym->name, sym->type);
    }

    if (arch_output->num_symbols >= 20) {
      printf("   │  ... (truncated)\n");
    }
    printf("   └────────────────\n");
  }
}

void pretty_print_macho(const struct machore_output_t *output, const char *path,
                        bool is_first_only, uint8_t display_flags) {
  if (output->is_fat && !is_first_only) {
    printf("📦 Fat Binary\n");
    printf("📂 Path: %s\n", path);
    printf("═══════════════════════\n\n");

    for (size_t arch_index = 0; arch_index < output->num_arch_outputs;
         arch_index++) {
      print_arch(&output->arch_outputs[arch_index], display_flags);
      printf("\n");
    }
  } else {
    printf("📦 Mach-O Binary\n");
    printf("📂 Path: %s\n", path);
    printf("══════════════\n");
    print_arch(&output->arch_outputs[0], display_flags);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  const char *filename = argv[1];

  bool is_first_only = false;
  uint8_t display_flags = 0;
  if (argc == 3) {
    const char *option = argv[2];
    if (strcmp(option, "--first-only") == 0) {
      is_first_only = true;
    } else if (strcmp(option, "--strings") == 0) {
      display_flags |= DISPLAY_STRINGS;
    } else if (strcmp(option, "--symbols") == 0) {
      display_flags |= DISPLAY_SYMBOLS;
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (argc == 4) {
    const char *option = argv[3];
    if (strcmp(option, "--first-only") == 0) {
      is_first_only = true;
    } else if (strcmp(option, "--strings") == 0) {
      display_flags |= DISPLAY_STRINGS;
    } else if (strcmp(option, "--symbols") == 0) {
      display_flags |= DISPLAY_SYMBOLS;
    } else {
      print_usage(argv[0]);
      return 1;
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

  struct machore_output_t output;
  init_output(&output);

  parse_macho(&output, buffer, size);
  pretty_print_macho(&output, filename, is_first_only, display_flags);

  free(buffer);
  clean_output(&output);
  return 0;
}
