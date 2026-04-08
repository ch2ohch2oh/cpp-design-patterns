add_executable(composite_example main.cpp)

set_target_properties(composite_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/composite
)

