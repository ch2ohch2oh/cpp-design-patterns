add_executable(strategy_example main.cpp)

set_target_properties(strategy_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/strategy
)
