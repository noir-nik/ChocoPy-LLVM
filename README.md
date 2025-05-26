## Using the compiler driver:

Build the project first

Compile a file:
```bash
chocopy-llvm input.py
```

To print the AST add the `--ast-dump` flag:
```bash
chocopy-llvm input.py --ast-dump
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
- LLVM

It is recommended to use clang and libc++

### Configuration:
With Custom LLVM build specify `-D LLVM_DIR=/path/to/llvm` if it was not found by CMake
```bash
cmake . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fno-rtti -fno-exceptions" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Build:
```bash
cmake --build build
```

### Run from the build directory:
```bash
./build/bin/chocopy-llvm
```

### Running tests:
```bash
cd Test
python run_test.py ./Parser/Stage_A -e ../build/bin/chocopy-llvm
python run_test.py ./Parser/Stage_B -e ../build/bin/chocopy-llvm
```
