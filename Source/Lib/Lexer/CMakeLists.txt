set(LLVM_LINK_COMPONENTS
  Support)

add_library(chocopy-llvm-lexer
    Lexer.cpp
    CachingLexer.cpp
    TokenKinds.cpp
)

target_link_libraries(chocopy-llvm-lexer PRIVATE
	chocopy-llvm-basic
)
