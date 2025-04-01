add_library(Lex
	Lexer.cpp
)

target_sources(Lex PUBLIC
	FILE_SET CXX_MODULES
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
	FILES Lexer.cppm
)