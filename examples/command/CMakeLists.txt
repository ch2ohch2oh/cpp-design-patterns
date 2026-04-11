add_executable(command_example main.cpp)

set_target_properties(command_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/command
)
