add_executable(reactor_example main.cpp)

set_target_properties(reactor_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/reactor
)
