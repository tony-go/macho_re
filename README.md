# QuickMacho

> ⚠️ **Note:** This project is currently under active development. Features and APIs may change.

QuickMacho is a lightweight command-line tool for parsing and analyzing Mach-O binary files on macOS. It displays information about linked libraries, CPU architectures, and version information for each dynamic library in the binary.

## Features

- Parse both single-architecture and fat (universal) Mach-O binaries
- Display CPU architecture information (x86, x86_64, ARM, ARM64)
- List all linked dynamic libraries
- Show version information for each dynamic library
- Support for various load commands (LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, LC_ID_DYLIB)

## Building

QuickMacho uses CMake as its build system. To build the project:

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
./quickmacho <path_to_macho_file>
```


```
❯ ./quickmacho /bin/ls
Number of architectures: 2
========================================
CPU Type: x86_64
Number of load commands: 18
Load dylib: /usr/lib/libutil.dylib - version: 0.1.0
Load dylib: /usr/lib/libncurses.5.4.dylib - version: 0.5.0
Load dylib: /usr/lib/libSystem.B.dylib - version: 5.65.2
========================================
CPU Type: ARM64
Number of load commands: 19
Load dylib: /usr/lib/libutil.dylib - version: 0.1.0
Load dylib: /usr/lib/libncurses.5.4.dylib - version: 0.5.0
Load dylib: /usr/lib/libSystem.B.dylib - version: 5.65.2
```
