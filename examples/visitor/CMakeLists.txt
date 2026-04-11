add_executable(visitor_example main.cpp)

set_target_properties(visitor_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/visitor
)
