add_executable(state_example main.cpp)

set_target_properties(state_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/state
)
