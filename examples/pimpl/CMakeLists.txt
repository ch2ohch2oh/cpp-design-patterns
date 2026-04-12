add_executable(pimpl_example main.cpp database_service.cpp)

set_target_properties(pimpl_example PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/pimpl
)
