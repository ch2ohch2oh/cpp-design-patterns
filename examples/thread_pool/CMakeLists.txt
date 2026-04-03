add_executable(thread_pool_example main.cpp)

set_target_properties(thread_pool_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/thread_pool
)
