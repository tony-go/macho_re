#include <mach-o/dyld.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmachore.h"

/*
 *
 *
 * PRIVATE APIS
 *
 *
 */

bool is_fat_header(uint8_t *buffer) {
  uint32_t magic = *(uint32_t *)buffer;
  return magic == FAT_MAGIC || magic == FAT_CIGAM || magic == FAT_MAGIC_64 ||
         magic == FAT_CIGAM_64;
}

void clean_arch_analysis(struct arch_analysis *arch_analysis) {
  free(arch_analysis->dylibs);
  arch_analysis->num_dylibs = 0;

  free(arch_analysis->strings);
  arch_analysis->num_strings = 0;
}

// The version is a 32-bit integer in the format 0xMMmmPPPP, where MM is the
// major version, mm is the minor version, and PPPP is the patch version.
// We need to extract these values and print them in the format MM.mm.PPPP.
bool parse_dylib_version(struct dylib_command *cmd, char *output_version_str,
                         size_t output_version_str_size) {
  uint32_t version = cmd->dylib.current_version;
  uint32_t major = (version >> 24) & 0xFF;
  uint32_t minor = (version >> 16) & 0xFF;
  uint32_t patch = version & 0xFF;
  // return a string in the format MM.mm.PPPP
  size_t written = snprintf(output_version_str, output_version_str_size,
                            "%u.%u.%u", major, minor, patch);
  return written >= output_version_str_size;
}

bool parse_dylib_name(struct dylib_command *cmd, char *output_name_str,
                      size_t output_name_str_size) {
  const char *name = (char *)cmd + cmd->dylib.name.offset;
  size_t written = snprintf(output_name_str, output_name_str_size, "%s", name);
  return written >= output_name_str_size;
}

void parse_dylib_command(struct dylib_command *dylib_cmd,
                         struct arch_analysis *arch_analysis) {
  arch_analysis->num_dylibs++;
  arch_analysis->dylibs =
      realloc(arch_analysis->dylibs,
              arch_analysis->num_dylibs * sizeof(struct dylib_info));

  struct dylib_info *dylib_info =
      &arch_analysis->dylibs[arch_analysis->num_dylibs - 1];

  char name_str[LIBMACHORE_DYLIB_PATH_SIZE];
  bool is_name_truncated =
      parse_dylib_name(dylib_cmd, name_str, LIBMACHORE_DYLIB_PATH_SIZE);
  dylib_info->is_path_truncated = is_name_truncated;
  strncpy(dylib_info->path, name_str, LIBMACHORE_DYLIB_PATH_SIZE);

  char version_str[LIBMACHORE_DYLIB_VERSION_SIZE];
  parse_dylib_version(dylib_cmd, version_str, LIBMACHORE_DYLIB_VERSION_SIZE);
  strncpy(dylib_info->version, version_str, LIBMACHORE_DYLIB_VERSION_SIZE);
}

#define PARSE_SECTION(arch_analysis, buffer, sect, segment_name)               \
  char *string_start = (char *)buffer + sect->offset;                          \
  char *string_end = string_start + sect->size;                                \
  char *string = string_start;                                                 \
  while (string < string_end) {                                                \
    const size_t string_length = strlen(string);                               \
    if (string_length > 0) {                                                   \
      arch_analysis->num_strings++;                                            \
      arch_analysis->strings =                                                 \
          realloc(arch_analysis->strings,                                      \
                  arch_analysis->num_strings * sizeof(struct string_info));    \
      struct string_info *string_info =                                        \
          &arch_analysis->strings[arch_analysis->num_strings - 1];             \
      assert(string_info != NULL);                                             \
      string_info->size = string_length + 1;                                   \
      string_info->content = malloc(string_info->size);                        \
      assert(string_info->content != NULL);                                    \
      strcpy(string_info->content, string);                                    \
      strncpy(string_info->original_segment, segment_name, 24);                \
      strncpy(string_info->original_section, sect->sectname, 24);              \
      ptrdiff_t relative_offset = string - string_start;                       \
      string_info->original_offset = sect->offset + relative_offset;           \
    }                                                                          \
    string += string_length + 1;                                               \
  }

void parse_text_segment64(struct arch_analysis *arch_analysis, uint8_t *buffer,
                          struct segment_command_64 *seg) {
  struct section_64 *sect = (void *)seg + sizeof(struct segment_command_64);
  for (uint32_t index = 0; index < seg->nsects; index++) {
    if (strcmp(sect->sectname, "__cstring") == 0 ||
        strcmp(sect->sectname, "__const") == 0 ||
        strcmp(sect->sectname, "__oslogstring") == 0) {
      PARSE_SECTION(arch_analysis, buffer, sect, "__TEXT");
    }
    sect++;
  }
}

void parse_text_segment(struct arch_analysis *arch_analysis, uint8_t *buffer,
                        struct segment_command *seg) {
  struct section *sect = (void *)seg + sizeof(struct segment_command);
  for (uint32_t index = 0; index < seg->nsects; index++) {
    if (strcmp(sect->sectname, "__cstring") == 0 ||
        strcmp(sect->sectname, "__const") == 0 ||
        strcmp(sect->sectname, "__oslogstring") == 0) {
      PARSE_SECTION(arch_analysis, buffer, sect, "__TEXT");
    }
    sect++;
  }
}

void parse_load_commands(struct arch_analysis *arch_analysis, uint8_t *buffer,
                         uint32_t ncmds) {
  struct mach_header *header = (struct mach_header *)buffer;
  uint32_t magic_header = header->magic;

  // Go to the first load command
  // account for the size of the mach header
  uint8_t *cmd;
  if (magic_header == MH_MAGIC_64) {
    cmd = buffer + sizeof(struct mach_header_64);
  } else {
    cmd = buffer + sizeof(struct mach_header);
  }

  for (uint32_t index = 0; index < ncmds; index++) {
    struct load_command *lc = (struct load_command *)cmd;

    switch (lc->cmd) {
    case LC_LOAD_DYLIB:
    case LC_LOAD_WEAK_DYLIB:
    case LC_ID_DYLIB:
    case LC_REEXPORT_DYLIB:
    case LC_LOAD_UPWARD_DYLIB:
    case LC_LAZY_LOAD_DYLIB: {
      // TODO: we should add context, like the original_load_command
      parse_dylib_command((struct dylib_command *)lc, arch_analysis);
      break;
    }
    case LC_SEGMENT_64: {
      struct segment_command_64 *seg = (struct segment_command_64 *)lc;
      if (strcmp(seg->segname, "__TEXT") == 0) {
        parse_text_segment64(arch_analysis, buffer, seg);
      }
    }
    case LC_SEGMENT: {
      struct segment_command *seg = (struct segment_command *)lc;
      if (strcmp(seg->segname, "__TEXT") == 0) {
        parse_text_segment(arch_analysis, buffer, seg);
      }
    }
    default:
      break;
    }

    cmd += lc->cmdsize;
  }
}

void copy_cpu_arch(uint32_t cpu_type, char *output_str,
                   size_t output_str_size) {
  switch (cpu_type) {
  case CPU_TYPE_X86:
    strncpy(output_str, "x86", output_str_size);
    break;
  case CPU_TYPE_X86_64:
    strncpy(output_str, "x86_64", output_str_size);
    break;
  case CPU_TYPE_ARM:
    strncpy(output_str, "ARM", output_str_size);
    break;
  case CPU_TYPE_ARM64:
    strncpy(output_str, "ARM64", output_str_size);
    break;
  default:
    strncpy(output_str, "Unknown", output_str_size);
  }
}

filetype_t get_file_type(uint32_t filetype) {
  switch (filetype) {
  case MH_DYLIB:
    return FILETYPE_DYLIB;
  case MH_EXECUTE:
    return FILETYPE_EXEC;
  case MH_BUNDLE:
    return FILETYPE_BUNDLE;
  case MH_CORE:
    return FILETYPE_CORE_DUMP;
  case MH_OBJECT:
    return FILETYPE_OBJECT;
  default:
    return FILETYPE_NOT_SUPPORTED;
  }
}

void parse_macho_arch(struct analysis *analysis, int arch_index,
                      uint8_t *buffer) {
  // 1. Allocate memory for the new arch_analysis struct
  // TODO(tonygo):
  // - Check if the realloc failed
  // - Offload this allocation to a function
  analysis->num_arch_analyses++;
  analysis->arch_analyses =
      realloc(analysis->arch_analyses,
              analysis->num_arch_analyses * sizeof(struct arch_analysis));

  // 2. Pick the the arch_analysis struct that we just allocated
  struct arch_analysis *arch_analysis = &analysis->arch_analyses[arch_index];

  // 3. Pick the macho header of this architecture
  struct mach_header *header = (struct mach_header *)buffer;

  // 4. Copy the architecture name
  uint32_t cpu_type = header->cputype;
  copy_cpu_arch(cpu_type, arch_analysis->architecture,
                LIBMACHORE_ARCHITECTURE_SIZE);

  // 5. Assin the filetype enum
  uint32_t filetype = header->filetype;
  arch_analysis->filetype = get_file_type(filetype);

  // 6. Parse the load commands
  uint32_t ncmds = header->ncmds;
  parse_load_commands(arch_analysis, buffer, ncmds);
}

/*
 *
 *
 * PUBLIC APIS
 *
 *
 */

void create_analysis(struct analysis *analysis) {
  analysis->arch_analyses = NULL;
  analysis->num_arch_analyses = 0;
  analysis->is_fat = false;
}

void clean_analysis(struct analysis *analysis) {
  for (size_t i = 0; i < analysis->num_arch_analyses; i++) {
    clean_arch_analysis(&analysis->arch_analyses[i]);
  }
  free(analysis->arch_analyses);
  analysis->arch_analyses = NULL;
  analysis->num_arch_analyses = 0;
  analysis->is_fat = false;
}

void parse_macho(struct analysis *analysis, uint8_t *buffer, size_t size) {
  bool is_fat = is_fat_header(buffer);
  if (is_fat) {
    analysis->is_fat = true;
    struct fat_header *header = (struct fat_header *)buffer;
    uint32_t nfat_arch = ntohl(header->nfat_arch);

    for (uint32_t arch_index = 0; arch_index < nfat_arch; arch_index++) {
      struct fat_arch *arch =
          (struct fat_arch *)(buffer + sizeof(struct fat_header) +
                              arch_index * sizeof(struct fat_arch));
      uint32_t offset = ntohl(arch->offset);
      parse_macho_arch(analysis, arch_index, buffer + offset);
    }
  } else {
    parse_macho_arch(analysis, 0, buffer);
  }
}
