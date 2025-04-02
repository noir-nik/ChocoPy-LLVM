set(LLVM_LINK_COMPONENTS
  Support)

add_library(chocopy-llvm-sema
    IdentifierResolver.cpp
    Sema.cpp
    Scope.cpp
)

target_link_libraries(chocopy-llvm-sema PRIVATE
    chocopy-llvm-basic
    chocopy-llvm-AST
    chocopy-llvm-analysis)
