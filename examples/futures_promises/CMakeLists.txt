add_executable(futures_promises_example main.cpp)

set_target_properties(futures_promises_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/futures_promises
)
