add_executable(observer_example main.cpp)

set_target_properties(observer_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/observer
)
