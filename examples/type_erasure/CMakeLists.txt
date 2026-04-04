add_executable(type_erasure_example main.cpp)

set_target_properties(type_erasure_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/type_erasure
)
