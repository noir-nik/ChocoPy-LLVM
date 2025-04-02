
set(DRIVER_NAME chocopy-llvm)

add_executable(${DRIVER_NAME} 
	Driver.cpp
)

target_link_libraries(${DRIVER_NAME} PRIVATE
	Utils
    # chocopy-llvm-AST
    # chocopy-llvm-analysis
    # chocopy-llvm-codegen
    # chocopy-llvm-sema
    # chocopy-llvm-parser
    chocopy-llvm-lexer
)