cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project(app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD 1)

# Include directories
set(INCLUDE_DIRS

)
include_directories(${INCLUDE_DIRS})

# LLVM
find_package(LLVM QUIET)
if (LLVM_FOUND)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM STATUS:
  Version       ${LLVM_VERSION}
  Includes      ${LLVM_INCLUDE_DIRS}
  Libraries     ${LLVM_LIBRARY_DIRS}
  Build type    ${LLVM_BUILD_TYPE}
  Dynamic lib   ${LLVM_LINK_LLVM_DYLIB}"
)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

if(LLVM_LINK_LLVM_DYLIB)
	message(STATUS "Linking to LLVM dynamic shared library object")
	set(llvm_libs LLVM)
else()
	message(STATUS "Linking to separate LLVM static libraries")
	llvm_map_components_to_libnames(llvm_libs
		Core
		Support
	)
endif()

# message(STATUS "llvm_libs: "${llvm_libs})
link_libraries(${llvm_libs})
	
else() # LLVM_FOUND
message(STATUS "LLVM package was not found")

if(NOT DEFINED LLVM_INCLUDE_DIR)
message(FATAL_ERROR "LLVM_INCLUDE_DIR is not set. Please specify the path to the LLVM include directory.
  Example (linux): -DLLVM_INCLUDE_DIR=\"/usr/lib/llvm-20/include\""
)
else() # LLVM_INCLUDE_DIR
message(STATUS "LLVM_INCLUDE_DIR: ${LLVM_INCLUDE_DIR}")
endif() # LLVM_INCLUDE_DIR

if(NOT DEFINED LLVM_LIBRARY_DIR)
message(FATAL_ERROR "LLVM_LIBRARY_DIR is not set. Please specify the path to the LLVM lib directory.
  Example (linux): -DLLVM_LIBRARY_DIR=\"/usr/lib/llvm-20/lib\""
)
else() # LLVM_LIBRARY_DIR
message(STATUS "LLVM_LIBRARY_DIR: ${LLVM_LIBRARY_DIR}")
endif() # LLVM_LIBRARY_DIR

if(NOT DEFINED LLVM_DLL)
message(FATAL_ERROR "LLVM_DLL is not set. Please specify the path to the LLVM DLL.
  Example (linux): -DLLVM_DLL=\"LLVM\"
  Example (win):   -DLLVM_DLL=\"LLVM-20\""
)
else() # LLVM_DLL
message(STATUS "LLVM_DLL: ${LLVM_DLL}")
endif() # LLVM_DLL

link_directories(${LLVM_LIBRARY_DIR})
include_directories(${LLVM_INCLUDE_DIR})
link_libraries(${LLVM_DLL})

endif() # LLVM_FOUND

add_subdirectory(Source)

# cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fno-rtti -fno-exceptions" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
