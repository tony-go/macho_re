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

