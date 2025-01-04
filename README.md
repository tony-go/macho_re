# macho_re

> ⚠️ **Note:** This project is currently under active development. Features and APIs may change.

`macho_re` is a lightweight command-line tool for parsing and analyzing Mach-O binary files on macOS. It displays information about linked libraries, CPU architectures, and version information for each dynamic library in the binary.

## Features

- Parse both single-architecture and fat (universal) Mach-O binaries
- Handle different binary types (executable, dylib, object file, etc.)
- List all linked **dynamic libraries** with versions
- Extract all **strings** with their locations
- Extract **symbols** and their types
- Show binary flags and security info (Code signing, entitlements)

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
❯ ./build/macho_re /Applications/Firefox.app/Contents/MacOS/firefox --first-only
📦 Mach-O Binary
📂 Path: /Applications/Firefox.app/Contents/MacOS/firefox
══════════════
🔧 Architecture: x86_64
📁 File Type: Executable
   ├─ Binary Flags:
   │  • No Undefined References: Yes
   │  • Dyld Compatible: Yes
   │  • Defines Weak Symbols: No
   │  • Uses Weak Symbols: Yes
   │  • Allows Stack Execution: No
   │  • Enforce No Heap Execution: No
   ├─ Security Flags:
   │  • Is Signed: Yes
   │  • Library Validation Disabled: Yes
   │  • Dylib Environment Variable allowed: No
   │  • Hardened Runtime: Yes
   ├─ Linked Libraries:
   │  • @rpath/libmozglue.dylib
   │   └─ Version: 0.1.0
   │  • /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation
   │   └─ Version: 9.116.0
   │  • /usr/lib/libc++.1.dylib
   │   └─ Version: 6.164.0
   │  • /usr/lib/libSystem.B.dylib
   │   └─ Version: 5.65.2
   ├─ String:
   │  • nsBrowserApp main (__TEXT,__cstring)
   │  • silentmode (__TEXT,__cstring)
   │  • MOZ_APP_SILENT_START=1 (__TEXT,__cstring)
   │  • MOZ_APP_NO_DOCK=1 (__TEXT,__cstring)
   │  • /dev/null (__TEXT,__cstring)
   │  • Couldn't find the application directory.\n (__TEXT,__cstring)
   │  • Couldn't load XPCOM.\n (__TEXT,__cstring)
   │  • MOZ_RELEASE_ASSERT(is<N>()) (__TEXT,__cstring)
   │  • XUL_APP_FILE (__TEXT,__cstring)
   │  • app (__TEXT,__cstring)
   │  • Incorrect number of arguments passed to -app (__TEXT,__cstring)
   │  • XUL_APP_FILE=%s (__TEXT,__cstring)
   │  • Couldn't set %s.\n (__TEXT,__cstring)
   │  • xpcshell (__TEXT,__cstring)
   │  • browser (__TEXT,__cstring)
   │  • Mozilla (__TEXT,__cstring)
   │  • Firefox (__TEXT,__cstring)
   │  • firefox (__TEXT,__cstring)
   │  • 133.0.3 (__TEXT,__cstring)
   │  • 20241209150345 (__TEXT,__cstring)
   │  ... (truncated)
   └────────────────
   ├─ Symbols:
   │  • __mh_execute_header (EXTERNAL)
   │  • _CFBundleCopyExecutableURL (EXTERNAL)
   │  • _CFBundleGetMainBundle (EXTERNAL)
   │  • _CFRelease (EXTERNAL)
   │  • _CFURLGetFileSystemRepresentation (EXTERNAL)
   │  • __ZN13CrashReporter30RegisterRuntimeExceptionModuleEv (EXTERNAL)
   │  • __ZN13CrashReporter32UnregisterRuntimeExceptionModuleEv (EXTERNAL)
   │  • __ZN7mozilla12PrintfTarget6vprintEPKcP13__va_list_tag (EXTERNAL)
   │  • __ZN7mozilla12PrintfTargetC2Ev (EXTERNAL)
   │  • __ZN7mozilla12baseprofiler13profiler_initEPv (EXTERNAL)
   │  • __ZN7mozilla12baseprofiler14ProfilingStack18ensureCapacitySlowEv (EXTERNAL)
   │  • __ZN7mozilla12baseprofiler17AutoProfilerLabel17GetProfilingStackEv (EXTERNAL)
   │  • __ZN7mozilla12baseprofiler17profiler_shutdownEv (EXTERNAL)
   │  • __ZN7mozilla12baseprofiler26profiler_current_thread_idEv (EXTERNAL)
   │  • __ZN7mozilla12baseprofiler6detail12RacyFeatures19IsActiveAndUnpausedEv (EXTERNAL)
   │  • __ZN7mozilla12baseprofiler9AddMarkerINS0_7markers10TextMarkerEJNSt3__112basic_stringIcNS4_11char_traitsIcEENS4_9allocatorIcEEEEEEENS_23ProfileBufferBlockIndexERKNS_18ProfilerStringViewIcEERKNS_14MarkerCategoryEONS_13MarkerOptionsET_DpRKT0_ (EXTERNAL)
   │  • __ZN7mozilla6detail9MutexImpl4lockEv (EXTERNAL)
   │  • __ZN7mozilla6detail9MutexImpl6unlockEv (EXTERNAL)
   │  • __ZN7mozilla6detail9MutexImplD2Ev (EXTERNAL)
   │  • __ZN7mozilla9TimeStamp3NowEb (EXTERNAL)
   │  ... (truncated)
   └────────────────
```

## C API

`macho_re` provides a C library (`libmachore`) that can be used to parse Mach-O binaries programmatically.

### Data Structures

Please check `lib/libmachore.h`!

### Functions

#### `void init_output(struct machore_output_t *output)`
Initializes a new analysis structure. Must be called before using the analysis structure.

#### `void clean_output(struct machore_output_t *output)`
Frees all resources associated with an analysis structure. Should be called when analysis is no longer needed.

#### `void parse_macho(struct machore_output_t *output, uint8_t *buffer, size_t size)`
Parses a Mach-O binary from a memory buffer.

- `output`: Pointer to an initialized output structure
- `buffer`: Pointer to the binary data
- `size`: Size of the binary data in bytes

### Example Usage

```c
// Read binary file into buffer
uint8_t *buffer = /* ... */;
size_t size = /* ... */;

// Initialize analysis
struct machore_output_t output;
init_output(&output);

// Parse the binary
parse_macho(&output, buffer, size);

// Use the results
for (size_t i = 0; i < output.num_arch_outputs; i++) {
    struct arch_analysis *arch_output = &output.arch_outputs[i];
    printf("Architecture: %s\n", arch_output->architecture);

    for (size_t j = 0; j < arch_output->num_dylibs; j++) {
        struct dylib_info *lib = &arch_output->dylibs[j];
        printf("Library: %s (version %s)\n", lib->path, lib->version);
    }
}

// Clean up
clean_output(&output);
```
