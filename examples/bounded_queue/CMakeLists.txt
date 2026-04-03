add_executable(bounded_queue_example main.cpp)

set_target_properties(bounded_queue_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/bounded_queue
)
