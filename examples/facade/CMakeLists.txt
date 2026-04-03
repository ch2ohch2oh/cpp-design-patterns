add_executable(facade_example main.cpp)

set_target_properties(facade_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/facade
)
