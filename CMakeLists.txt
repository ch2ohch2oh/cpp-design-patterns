cmake_minimum_required(VERSION 3.20)

project(cpp_backend_patterns_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(examples/raii)
add_subdirectory(examples/smart_pointers)
add_subdirectory(examples/rule_of_0_5)

