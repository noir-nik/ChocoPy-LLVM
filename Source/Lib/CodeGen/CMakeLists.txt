set(LLVM_LINK_COMPONENTS
  Core
  Support)

add_library(chocopy-llvm-codegen
  ModuleBuilder.cpp
  CodeGenModule.cpp
  CodeGenFunction.cpp

)

target_link_libraries(chocopy-llvm-codegen PRIVATE
    chocopy-llvm-AST
)
