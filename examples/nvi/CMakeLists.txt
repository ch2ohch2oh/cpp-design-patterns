add_executable(nvi_example main.cpp)

set_target_properties(nvi_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/nvi
)
