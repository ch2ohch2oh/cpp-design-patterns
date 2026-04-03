add_executable(proxy_example main.cpp)

set_target_properties(proxy_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/proxy
)
