cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project(app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD 1)

# Main executable
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "Source/*.cpp")

set(ALL_SOURCES
	${SOURCES}
)
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# Include directories
set(INCLUDE_DIRS

)
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# C++ modules
file(GLOB_RECURSE MODULE_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "Source/*.cppm")

target_sources(${PROJECT_NAME} PUBLIC
	FILE_SET CXX_MODULES
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${MODULE_SOURCES}
)

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
target_include_directories(${PROJECT_NAME} PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LLVM_LIBRARY_DIRS})

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
target_link_libraries(${PROJECT_NAME} PRIVATE ${llvm_libs})
	
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

target_link_directories(${PROJECT_NAME} PRIVATE ${LLVM_LIBRARY_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${LLVM_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_DLL})

endif() # LLVM_FOUND

# cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fno-rtti -fno-exceptions" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
