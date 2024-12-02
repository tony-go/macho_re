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
❯ ./build/macho_re /bin/ls
📦 Fat Binary
📂 Path: /bin/ls
═══════════════════════

🔧 Architecture: x86_64
   ├─ Linked Libraries:
   │  • /usr/lib/libutil.dylib
   │   └─ Version: 0.1.0
   │  • /usr/lib/libncurses.5.4.dylib
   │   └─ Version: 0.5.0
   │  • /usr/lib/libSystem.B.dylib
   │   └─ Version: 5.71.0
   └────────────────

🔧 Architecture: ARM64
   ├─ Linked Libraries:
   │  • /usr/lib/libutil.dylib
   │   └─ Version: 0.1.0
   │  • /usr/lib/libncurses.5.4.dylib
   │   └─ Version: 0.5.0
   │  • /usr/lib/libSystem.B.dylib
   │   └─ Version: 5.71.0
   └────────────────
```

## C API

`macho_re` provides a C library (`libmachore`) that can be used to parse Mach-O binaries programmatically.

### Data Structures

```c
struct dylib_info {
    char path[LIBMACHORE_DYLIB_PATH_SIZE];     // Path to the dynamic library
    bool is_path_truncated;                     // True if path was truncated
    char version[LIBMACHORE_DYLIB_VERSION_SIZE]; // Version string (format: MM.mm.PPPP)
    bool is_version_truncated;                  // True if version was truncated
};

struct arch_analysis {
    char architecture[LIBMACHORE_ARCHITECTURE_SIZE]; // CPU architecture (x86, x86_64, ARM, ARM64)
    struct dylib_info *dylibs;                      // Array of dynamic libraries
    size_t num_dylibs;                              // Number of dynamic libraries
};

struct analysis {
    struct arch_analysis *arch_analyses;    // Array of architecture analyses
    size_t num_arch_analyses;               // Number of architectures
    bool is_fat;                            // True if binary is a fat/universal binary
};
```

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
