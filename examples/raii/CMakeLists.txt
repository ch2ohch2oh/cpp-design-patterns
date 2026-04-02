add_executable(raii_example main.cpp)

set_target_properties(raii_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/raii
)

