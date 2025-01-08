#include <libkern/OSByteOrder.h>
#include <mach-o/dyld.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cs_blobs_shim.h"
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

void clean_arch_output(struct machore_arch_output_t *arch_output) {
  // Binary flags
  arch_output->no_undefined_refs = false;
  arch_output->dyld_compatible = false;
  arch_output->defines_weak_symbols = false;
  arch_output->uses_weak_symbols = false;
  arch_output->allows_stack_execution = false;
  arch_output->enforce_no_heap_exec = false;

  // Security flags
  if (arch_output->security_flags != NULL) {
    free(arch_output->security_flags);
  }

  // TODO: we should free the memory of the dylibs, strings
  // and symbols by iterating over the arrays and freeing
  // each element.
  free(arch_output->dylibs);
  arch_output->num_dylibs = 0;

  free(arch_output->strings);
  arch_output->num_strings = 0;

  free(arch_output->symbols);
  arch_output->num_symbols = 0;

  if (arch_output->entitlements != NULL) {
    free(arch_output->entitlements);
  }
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
                         struct machore_arch_output_t *arch_output) {
  arch_output->num_dylibs++;
  arch_output->dylibs = realloc(
      arch_output->dylibs, arch_output->num_dylibs * sizeof(struct dylib_info));

  struct dylib_info *dylib_info =
      &arch_output->dylibs[arch_output->num_dylibs - 1];

  char name_str[LIBMACHORE_DYLIB_PATH_SIZE];
  bool is_name_truncated =
      parse_dylib_name(dylib_cmd, name_str, LIBMACHORE_DYLIB_PATH_SIZE);
  dylib_info->is_path_truncated = is_name_truncated;
  strncpy(dylib_info->path, name_str, LIBMACHORE_DYLIB_PATH_SIZE);

  char version_str[LIBMACHORE_DYLIB_VERSION_SIZE];
  parse_dylib_version(dylib_cmd, version_str, LIBMACHORE_DYLIB_VERSION_SIZE);
  strncpy(dylib_info->version, version_str, LIBMACHORE_DYLIB_VERSION_SIZE);
}

#define PARSE_SECTION(arch_output, buffer, sect, segment_name)                 \
  char *string_start = (char *)buffer + sect->offset;                          \
  char *string_end = string_start + sect->size;                                \
  char *string = string_start;                                                 \
  while (string < string_end) {                                                \
    const size_t string_length = strlen(string);                               \
    if (string_length > 0) {                                                   \
      arch_output->num_strings++;                                              \
      arch_output->strings =                                                   \
          realloc(arch_output->strings,                                        \
                  arch_output->num_strings * sizeof(struct string_info));      \
      struct string_info *string_info =                                        \
          &arch_output->strings[arch_output->num_strings - 1];                 \
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

void parse_text_segment64(struct machore_arch_output_t *arch_output,
                          uint8_t *buffer, struct segment_command_64 *seg) {
  struct section_64 *sect = (void *)seg + sizeof(struct segment_command_64);
  for (uint32_t index = 0; index < seg->nsects; index++) {
    if (strcmp(sect->sectname, "__cstring") == 0 ||
        strcmp(sect->sectname, "__const") == 0 ||
        strcmp(sect->sectname, "__oslogstring") == 0) {
      PARSE_SECTION(arch_output, buffer, sect, "__TEXT");
    }
    sect++;
  }
}

void parse_text_segment(struct machore_arch_output_t *arch_output,
                        uint8_t *buffer, struct segment_command *seg) {
  struct section *sect = (void *)seg + sizeof(struct segment_command);
  for (uint32_t index = 0; index < seg->nsects; index++) {
    if (strcmp(sect->sectname, "__cstring") == 0 ||
        strcmp(sect->sectname, "__const") == 0 ||
        strcmp(sect->sectname, "__oslogstring") == 0) {
      PARSE_SECTION(arch_output, buffer, sect, "__TEXT");
    }
    sect++;
  }
}

void parse_data_segment64(struct machore_arch_output_t *arch_output,
                          uint8_t *buffer, struct segment_command_64 *seg) {
  struct section_64 *sect = (void *)seg + sizeof(struct segment_command_64);
  for (uint32_t index = 0; index < seg->nsects; index++) {
    if (strcmp(sect->sectname, "__const") == 0 ||
        strcmp(sect->sectname, "__cfstring") == 0) {
      PARSE_SECTION(arch_output, buffer, sect, "__DATA");
    }
    sect++;
  }
}

void parse_data_segment(struct machore_arch_output_t *arch_output,
                        uint8_t *buffer, struct segment_command *seg) {
  struct section *sect = (void *)seg + sizeof(struct segment_command);
  for (uint32_t index = 0; index < seg->nsects; index++) {
    if (strcmp(sect->sectname, "__const") == 0 ||
        strcmp(sect->sectname, "__cfstring") == 0) {
      PARSE_SECTION(arch_output, buffer, sect, "__DATA");
    }
    sect++;
  }
}

void parse_data_const_segment64(struct machore_arch_output_t *arch_output,
                                uint8_t *buffer,
                                struct segment_command_64 *seg) {
  struct section_64 *sect = (void *)seg + sizeof(struct segment_command_64);
  for (uint32_t index = 0; index < seg->nsects; index++) {
    if (strcmp(sect->sectname, "__const") == 0) {
      PARSE_SECTION(arch_output, buffer, sect, "__DATA_CONST");
    }
    sect++;
  }
}

void parse_data_const_segment(struct machore_arch_output_t *arch_output,
                              uint8_t *buffer, struct segment_command *seg) {
  struct section *sect = (void *)seg + sizeof(struct segment_command);
  for (uint32_t index = 0; index < seg->nsects; index++) {
    if (strcmp(sect->sectname, "__const") == 0) {
      PARSE_SECTION(arch_output, buffer, sect, "__DATA_CONST");
    }
    sect++;
  }
}

void parse_entitlements(CS_GenericBlob_shim *entitlements_blob,
                        struct machore_arch_output_t *arch_output,
                        bool should_swap) {
  // Extract the entitlements XML
  uint32_t length = should_swap ? OSSwapInt32(entitlements_blob->length)
                                : entitlements_blob->length;
  uint32_t xml_length = length - sizeof(CS_GenericBlob_shim);
  char *entitlements = strndup((char *)entitlements_blob->data, xml_length);

  arch_output->entitlements = malloc(xml_length + 1);
  strcpy(arch_output->entitlements, entitlements);

  // Detect sensitive entitlements
  if (strstr(entitlements,
             "<key>com.apple.security.cs.disable-library-validation</key>") !=
          NULL &&
      strstr(entitlements, "<true/>") != NULL) {
    arch_output->security_flags->is_library_validation_disabled = true;
  }
}

void parse_codesign_flags(uint32_t raw_flags,
                          struct security_flags *security_flags,
                          bool should_swap) {
  uint32_t flags = should_swap ? OSSwapInt32(raw_flags) : raw_flags;
  if (flags & CS_RUNTIME) {
    security_flags->has_hardened_runtime = true;
  }
}

void parse_security_flags(struct machore_arch_output_t *arch_output,
                          uint8_t *buffer,
                          struct linkedit_data_command *linkedit_data_cmd) {
  // Allocate memory for the security_flags
  // and get the pointer to the security_flags
  arch_output->security_flags = malloc(sizeof(struct security_flags));
  struct security_flags *security_flags = arch_output->security_flags;

  security_flags->is_signed = true;

  // Get the pointer to the code slot and cast it to a SuperBlob
  uint8_t *code_slot = buffer + linkedit_data_cmd->dataoff;
  CS_SuperBlob_shim *super_blob = (CS_SuperBlob_shim *)code_slot;

  bool should_swap = super_blob->magic != CSMAGIC_EMBEDDED_SIGNATURE;
  uint32_t count =
      should_swap ? OSSwapInt32(super_blob->count) : super_blob->count;

  for (uint16_t index = 0; index < count; index++) {
    CS_BlobIndex_shim *blob_index = &super_blob->index[index];
    uint32_t type =
        should_swap ? OSSwapInt32(blob_index->type) : blob_index->type;
    uint32_t offset =
        should_swap ? OSSwapInt32(blob_index->offset) : blob_index->offset;
    switch (type) {
    case CSSLOT_CODEDIRECTORY: {
      CS_CodeDirectory_shim *code_directory =
          (CS_CodeDirectory_shim *)(code_slot + offset);
      parse_codesign_flags(code_directory->flags, security_flags, should_swap);
      break;
    }
    case CSSLOT_REQUIREMENTS:
      // TODO: see what we could extract from the requirements
      break;
    case CSSLOT_ENTITLEMENTS: {
      CS_GenericBlob_shim *entitlements_blob =
          (CS_GenericBlob_shim *)(code_slot + offset);

      uint32_t magic = should_swap ? OSSwapInt32(entitlements_blob->magic)
                                   : entitlements_blob->magic;
      if (magic != CSMAGIC_EMBEDDED_ENTITLEMENTS) {
        printf("Invalid magic number for entitlements\n");
        break;
      }
      // read only the size of the blob
      parse_entitlements(entitlements_blob, arch_output, should_swap);
      break;
    }
    }
  }
}

void parse_symtab(struct machore_arch_output_t *arch_output, uint8_t *buffer,
                  struct symtab_command *symtab_cmd) {
  char *str_symbol_table = (char *)buffer + symtab_cmd->stroff;
  struct nlist_64 *symbol_table_start =
      (struct nlist_64 *)(buffer + symtab_cmd->symoff);

  for (uint32_t index = 0; index < symtab_cmd->nsyms; index++) {
    struct nlist_64 *symbol = &symbol_table_start[index];
    if (symbol->n_un.n_strx == 0) {
      continue;
    }

    arch_output->num_symbols++;
    arch_output->symbols =
        realloc(arch_output->symbols,
                arch_output->num_symbols * sizeof(struct symbol_info));
    struct symbol_info *symbol_info =
        &arch_output->symbols[arch_output->num_symbols - 1];

    char *symbol_name = str_symbol_table + symbol->n_un.n_strx;
    symbol_info->name = symbol_name;

    if (symbol->n_sect == NO_SECT) {
      symbol_info->has_no_section = true;
    } else {
      symbol_info->has_no_section = false;
    }

    // TODO: handle symbol type N_TYPE
    // (with #include <mach-o/stab.h> for stabs)
    uint8_t type = symbol->n_type;
    if (type & N_STAB) {
      strcpy(symbol_info->type, "STAB");
    } else if (type & N_EXT) {
      strcpy(symbol_info->type, "EXTERNAL");
    } else {
      strcpy(symbol_info->type, "PRIVATE EXTERNAL");
    }
  }
}

void parse_load_commands(struct machore_arch_output_t *arch_output,
                         uint8_t *buffer, uint32_t ncmds) {
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
      parse_dylib_command((struct dylib_command *)lc, arch_output);
      break;
    }
    // TODO: handle other __LINKEDIT segments
    case LC_SEGMENT_64: {
      struct segment_command_64 *seg = (struct segment_command_64 *)lc;
      if (strcmp(seg->segname, "__TEXT") == 0) {
        parse_text_segment64(arch_output, buffer, seg);
      } else if (strcmp(seg->segname, "__DATA") == 0) {
        parse_data_segment64(arch_output, buffer, seg);
      } else if (strcmp(seg->segname, "__DATA_CONST") == 0) {
        parse_data_const_segment64(arch_output, buffer, seg);
      }
      break;
    }
    case LC_SEGMENT: {
      struct segment_command *seg = (struct segment_command *)lc;
      if (strcmp(seg->segname, "__TEXT") == 0) {
        parse_text_segment(arch_output, buffer, seg);
      } else if (strcmp(seg->segname, "__DATA") == 0) {
        parse_data_segment(arch_output, buffer, seg);
      } else if (strcmp(seg->segname, "__DATA_CONST") == 0) {
        parse_data_const_segment(arch_output, buffer, seg);
      }
      break;
    }
    case LC_CODE_SIGNATURE: {
      struct linkedit_data_command *linkedit_data_cmd =
          (struct linkedit_data_command *)lc;
      parse_security_flags(arch_output, buffer, linkedit_data_cmd);
      break;
    }
    case LC_SYMTAB: {
      struct symtab_command *symtab_cmd = (struct symtab_command *)lc;
      parse_symtab(arch_output, buffer, symtab_cmd);
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
  case MH_OBJECT:
    return LIBMACHORE_FILETYPE_OBJECT;
  case MH_EXECUTE:
    return LIBMACHORE_FILETYPE_EXECUTE;
  case MH_FVMLIB:
    return LIBMACHORE_FILETYPE_FVMLIB;
  case MH_CORE:
    return LIBMACHORE_FILETYPE_CORE;
  case MH_PRELOAD:
    return LIBMACHORE_FILETYPE_PRELOAD;
  case MH_DYLIB:
    return LIBMACHORE_FILETYPE_DYLIB;
  case MH_DYLINKER:
    return LIBMACHORE_FILETYPE_DYLINKER;
  case MH_BUNDLE:
    return LIBMACHORE_FILETYPE_BUNDLE;
  case MH_DYLIB_STUB:
    return LIBMACHORE_FILETYPE_DYLIB_STUB;
  case MH_DSYM:
    return LIBMACHORE_FILETYPE_DSYM;
  case MH_KEXT_BUNDLE:
    return LIBMACHORE_FILETYPE_KEXT_BUNDLE;
  default:
    return LIBMACHORE_FILETYPE_NOT_SUPPORTED;
  }
}

void parse_flags(uint32_t flags, struct machore_arch_output_t *arch_output) {
  arch_output->no_undefined_refs = flags & MH_NOUNDEFS;
  arch_output->dyld_compatible = flags & MH_DYLDLINK;
  arch_output->defines_weak_symbols = flags & MH_WEAK_DEFINES;
  arch_output->uses_weak_symbols = flags & MH_BINDS_TO_WEAK;
  arch_output->allows_stack_execution = flags & MH_ALLOW_STACK_EXECUTION;
  arch_output->enforce_no_heap_exec = flags & MH_NO_HEAP_EXECUTION;
}

void parse_macho_arch(struct machore_output_t *output, int arch_index,
                      uint8_t *buffer) {
  // 1. Allocate memory for the new arch_output struct
  // TODO(tonygo):
  // - Check if the realloc failed
  // - Offload this allocation to a function
  output->num_arch_outputs++;
  output->arch_outputs =
      realloc(output->arch_outputs,
              output->num_arch_outputs * sizeof(struct machore_arch_output_t));

  // 2. Pick the the arch_output struct that we just allocated
  struct machore_arch_output_t *arch_output = &output->arch_outputs[arch_index];

  // 3. Pick the macho header of this architecture
  struct mach_header *header = (struct mach_header *)buffer;

  // 4. Copy the architecture name
  uint32_t cpu_type = header->cputype;
  copy_cpu_arch(cpu_type, arch_output->architecture,
                LIBMACHORE_ARCHITECTURE_SIZE);

  // 5. Assin the filetype enum
  uint32_t filetype = header->filetype;
  arch_output->filetype = get_file_type(filetype);

  // 6. Parse the load commands
  uint32_t ncmds = header->ncmds;
  parse_load_commands(arch_output, buffer, ncmds);

  // 7. Parse flags
  parse_flags(header->flags, arch_output);
}

/*
 *
 *
 * PUBLIC APIS
 *
 *
 */

void init_output(struct machore_output_t *analysis) {
  analysis->arch_outputs = NULL;
  analysis->num_arch_outputs = 0;
  analysis->is_fat = false;
}

void clean_output(struct machore_output_t *output) {
  for (size_t i = 0; i < output->num_arch_outputs; i++) {
    clean_arch_output(&output->arch_outputs[i]);
  }
  free(output->arch_outputs);
  output->arch_outputs = NULL;
  output->num_arch_outputs = 0;
  output->is_fat = false;
}

void parse_macho(struct machore_output_t *output, uint8_t *buffer,
                 size_t size) {
  bool is_fat = is_fat_header(buffer);
  if (is_fat) {
    output->is_fat = true;
    struct fat_header *header = (struct fat_header *)buffer;
    uint32_t nfat_arch = ntohl(header->nfat_arch);

    for (uint32_t arch_index = 0; arch_index < nfat_arch; arch_index++) {
      struct fat_arch *arch =
          (struct fat_arch *)(buffer + sizeof(struct fat_header) +
                              arch_index * sizeof(struct fat_arch));
      uint32_t offset = ntohl(arch->offset);
      parse_macho_arch(output, arch_index, buffer + offset);
    }
  } else {
    parse_macho_arch(output, 0, buffer);
  }
}
