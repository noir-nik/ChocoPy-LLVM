
set(DRIVER_NAME chocopy-llvm)

add_executable(${DRIVER_NAME} 
	Driver.cpp
)

target_link_libraries(${DRIVER_NAME} PRIVATE
	Utils
)