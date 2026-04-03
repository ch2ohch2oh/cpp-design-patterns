add_executable(decorator_example main.cpp)

set_target_properties(decorator_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/decorator
)
