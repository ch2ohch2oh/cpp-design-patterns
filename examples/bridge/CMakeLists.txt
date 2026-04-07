add_executable(bridge_example main.cpp)

set_target_properties(bridge_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/bridge
)

