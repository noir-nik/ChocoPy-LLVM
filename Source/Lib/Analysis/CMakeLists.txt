set(LLVM_LINK_COMPONENTS
  Support)

add_library(chocopy-llvm-analysis
  CFG.cpp
  CFGPrinter.cpp
)

target_link_libraries(chocopy-llvm-analysis PRIVATE
    chocopy-llvm-basic
    chocopy-llvm-AST)
