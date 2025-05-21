add_subdirectory(Parser)
add_subdirectory(Sema)

add_custom_target(check-chpy)
add_dependencies(check-chpy
  check-chpy-parser
  check-chpy-sema)
