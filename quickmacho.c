#include <mach-o/dyld.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION_STR_SIZE 16
#define NAME_STR_SIZE 256

// The version is a 32-bit integer in the format 0xMMmmPPPP, where MM is the
// major version, mm is the minor version, and PPPP is the patch version.
// We need to extract these values and print them in the format MM.mm.PPPP.
bool parse_dylib_version(struct dylib_command *cmd, char *output_version_str, size_t output_version_str_size)
{
  uint32_t version = cmd->dylib.current_version;
  uint32_t major = (version >> 24) & 0xFF;
  uint32_t minor = (version >> 16) & 0xFF;
  uint32_t patch = version & 0xFF;
  char *version_str = malloc(10);
  // return a string in the format MM.mm.PPPP
  size_t written = snprintf(output_version_str, output_version_str_size, "%u.%u.%u", major,
                            minor, patch);
  return written >= output_version_str_size;
}

bool parse_dylib_name(struct dylib_command *cmd, char *output_name_str, size_t output_name_str_size)
{
  const char *name = (char *)cmd + cmd->dylib.name.offset;
  size_t written = snprintf(output_name_str, output_name_str_size, "%s", name);
  return written >= output_name_str_size;
}

void parse_load_commands(uint8_t *buffer, uint32_t ncmds)
{
  struct mach_header *header = (struct mach_header *)buffer;
  uint32_t magic_header = header->magic;

  uint8_t *cmd;
  if (magic_header == MH_MAGIC_64)
  {
    cmd = buffer + sizeof(struct mach_header_64);
  }
  else
  {
    cmd = buffer + sizeof(struct mach_header);
  }

  for (uint32_t index = 0; index < ncmds; index++)
  {
    struct load_command *lc = (struct load_command *)cmd;

    switch (lc->cmd)
    {
    case LC_LOAD_DYLIB:
    case LC_LOAD_WEAK_DYLIB:
    case LC_ID_DYLIB:
    case LC_REEXPORT_DYLIB:
    case LC_LOAD_UPWARD_DYLIB:
    case LC_LAZY_LOAD_DYLIB:
    {
      struct dylib_command *dylib_cmd = (struct dylib_command *)cmd;

      char name_str[NAME_STR_SIZE];
      bool is_name_truncated = parse_dylib_name(dylib_cmd, name_str, NAME_STR_SIZE);
      printf("Load dylib: %s ", name_str);
      if (is_name_truncated)
      {
        printf("(truncated) ");
      }
      printf("- ");

      char version_str[VERSION_STR_SIZE];
      bool is_version_truncated = parse_dylib_version(dylib_cmd, version_str, VERSION_STR_SIZE);
      printf("version: %s", version_str);
      if (is_version_truncated)
      {
        printf("(truncated) ");
      }
      printf("\n");
      break;
    }
    default:
      break;
    }

    cmd += lc->cmdsize;
  }
}

void parse_mach_o(uint8_t *buffer)
{
  struct mach_header *header = (struct mach_header *)buffer;
  uint32_t cpu_type = header->cputype;

  printf("========================================\n");
  switch (cpu_type)
  {
  case CPU_TYPE_X86:
    printf("CPU Type: x86\n");
    break;
  case CPU_TYPE_X86_64:
    printf("CPU Type: x86_64\n");
    break;
  case CPU_TYPE_ARM:
    printf("CPU Type: ARM\n");
    break;
  case CPU_TYPE_ARM64:
    printf("CPU Type: ARM64\n");
    break;
  default:
    printf("CPU Type: Unknown\n");
  }

  uint32_t ncmds = header->ncmds;
  printf("Number of load commands: %u\n", ncmds);

  parse_load_commands(buffer, ncmds);
}

void parse_fat(uint8_t *buffer, size_t size)
{
  struct fat_header *header = (struct fat_header *)buffer;
  uint32_t nfat_arch = ntohl(header->nfat_arch);
  printf("Number of architectures: %u\n", nfat_arch);

  for (uint32_t i = 0; i < nfat_arch; i++)
  {
    struct fat_arch *arch =
        (struct fat_arch *)(buffer + sizeof(struct fat_header) +
                            i * sizeof(struct fat_arch));
    uint32_t offset = ntohl(arch->offset);
    parse_mach_o(buffer + offset);
  }
}