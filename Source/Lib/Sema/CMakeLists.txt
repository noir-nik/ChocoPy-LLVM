set(LIBRARY_NAME chocopy-llvm-sema)

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

target_link_libraries(${LIBRARY_NAME} PRIVATE
	chocopy-llvm-basic
	chocopy-llvm-AST
)
