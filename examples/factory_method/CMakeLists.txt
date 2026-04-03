add_executable(factory_method_example main.cpp)

set_target_properties(factory_method_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/factory_method
)
