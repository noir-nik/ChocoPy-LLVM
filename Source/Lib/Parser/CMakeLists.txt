set(LIBRARY_NAME chocopy-llvm-parser)

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "Private/*.cpp")

add_library(${LIBRARY_NAME}
	${SOURCES}
)

file(GLOB MODULE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "Public/*.cppm")

target_sources(${LIBRARY_NAME} PUBLIC
	FILE_SET CXX_MODULES
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${MODULE_SOURCES}
)

target_link_libraries(chocopy-llvm-parser PRIVATE
	chocopy-llvm-basic
	chocopy-llvm-lexer
	chocopy-llvm-AST
	chocopy-llvm-sema
)
