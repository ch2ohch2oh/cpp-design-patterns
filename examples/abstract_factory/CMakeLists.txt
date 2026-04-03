add_executable(abstract_factory_example main.cpp)

set_target_properties(abstract_factory_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/abstract_factory
)
