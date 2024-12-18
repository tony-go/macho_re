# macho_re

> ⚠️ **Note:** This project is currently under active development. Features and APIs may change.

`macho_re` is a lightweight command-line tool for parsing and analyzing Mach-O binary files on macOS. It displays information about linked libraries, CPU architectures, and version information for each dynamic library in the binary.

## Features

- Parse both single-architecture and fat (universal) Mach-O binaries
- Display CPU architecture information (x86, x86_64, ARM, ARM64)
- List all linked dynamic libraries
- Show version information for each dynamic library
- Support for various load commands (LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, LC_ID_DYLIB)

## Building

`macho_re` uses CMake as its build system. To build the project:

```bash
make
```

## Usage

### CLI

```bash
./build/macho_re <path_to_macho_file>
```

```
❯ ./build/macho_re /bin/ls --first-only
📦 Mach-O Binary
📂 Path: /bin/ls
══════════════
🔧 Architecture: x86_64
📁 File Type: Executable
   ├─ Linked Libraries:
   │  • /usr/lib/libutil.dylib
   │   └─ Version: 0.1.0
   │  • /usr/lib/libncurses.5.4.dylib
   │   └─ Version: 0.5.0
   │  • /usr/lib/libSystem.B.dylib
   │   └─ Version: 5.71.0
   ├─ String:
   │  • @(#)PROGRAM:ls  PROJECT:file_cmds-448.0.3\n (__TEXT,__const)
   │  • |@$FreeBSD$ (__TEXT,__const)
   │  ...
   │  • search (__TEXT,__cstring)
   │  • delete (__TEXT,__cstring)
   │  • append (__TEXT,__cstring)
   │  • add_subdirectory (__TEXT,__cstring)
   │  • delete_child (__TEXT,__cstring)
   │  • readattr (__TEXT,__cstring)
   │  • writeattr (__TEXT,__cstring)
   │  • readextattr (__TEXT,__cstring)
   │  • writeextattr (__TEXT,__cstring)
   │  • readsecurity (__TEXT,__cstring)
   │  • writesecurity (__TEXT,__cstring)
   │  • chown (__TEXT,__cstring)
   │  • file_inherit (__TEXT,__cstring)
   │  • directory_inherit (__TEXT,__cstring)
   │  • % (__TEXT,__const)
   └────────────────


## C API

`macho_re` provides a C library (`libmachore`) that can be used to parse Mach-O binaries programmatically.

### Data Structures

Please check `lib/libmachore.h`!

### Functions

#### `void create_analysis(struct analysis *analysis)`
Initializes a new analysis structure. Must be called before using the analysis structure.

#### `void clean_analysis(struct analysis *analysis)`
Frees all resources associated with an analysis structure. Should be called when analysis is no longer needed.

#### `void parse_macho(struct analysis *analysis, uint8_t *buffer, size_t size)`
Parses a Mach-O binary from a memory buffer.

- `analysis`: Pointer to an initialized analysis structure
- `buffer`: Pointer to the binary data
- `size`: Size of the binary data in bytes

### Example Usage

```c
// Read binary file into buffer
uint8_t *buffer = /* ... */;
size_t size = /* ... */;

// Initialize analysis
struct analysis analysis;
create_analysis(&analysis);

// Parse the binary
parse_macho(&analysis, buffer, size);

// Use the results
for (size_t i = 0; i < analysis.num_arch_analyses; i++) {
    struct arch_analysis *arch = &analysis.arch_analyses[i];
    printf("Architecture: %s\n", arch->architecture);

    for (size_t j = 0; j < arch->num_dylibs; j++) {
        struct dylib_info *lib = &arch->dylibs[j];
        printf("Library: %s (version %s)\n", lib->path, lib->version);
    }
}

// Clean up
clean_analysis(&analysis);
```
