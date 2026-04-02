add_executable(smart_pointers_example main.cpp)

set_target_properties(smart_pointers_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/smart_pointers
)

