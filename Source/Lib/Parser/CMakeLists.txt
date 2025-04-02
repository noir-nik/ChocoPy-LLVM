set(LLVM_LINK_COMPONENTS
  Support)

add_library(chocopy-llvm-parser
    Parser.cpp
)

target_link_libraries(chocopy-llvm-parser PRIVATE
	chocopy-llvm-basic
    chocopy-llvm-lexer
    chocopy-llvm-AST
    chocopy-llvm-sema)
