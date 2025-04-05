## Using the compiler driver:

Build the project first

Compile a file:
```bash
chocopy-llvm input.py
```

To print the AST add the `-ast-dump` flag:
```bash
chocopy-llvm input.py -ast-dump
```

Compile a demo file
```bash
chocopy-llvm ./Test/Demo/demo.py
```

## Building the project:

### Requirements

- Compiler with C++23 support
- Toolchain with C++23 import std;
- CMake with Ninja build system

It is recommended to use clang and libc++

### Configuration:
With Custom LLVM build
```bash
cmake . -B build -G Ninja -D LLVM_DIR=/path/to/llvm
```

With LLVM headers and dynamic library
```bash
cmake . -B build -G Ninja -D LLVM_INCLUDE_DIR=/path/to/llvm/include -D LLVM_LIBRARY_DIR=/path/to/llvm/lib -D LLVM_DYN_LIB=LLVM.a
```

### Build:
```bash
cmake --build build
```
