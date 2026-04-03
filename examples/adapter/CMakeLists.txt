add_executable(adapter_example main.cpp)

set_target_properties(adapter_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/adapter
)
