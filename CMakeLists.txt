cmake_minimum_required(VERSION 3.10)
project(HelloWorld)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Release")

find_package(fmt CONFIG REQUIRED)

add_library(multidim src/multidim.cpp)
target_link_libraries(multidim PRIVATE fmt::fmt)
target_include_directories(multidim PUBLIC "deps/gch" "include")
target_compile_options(multidim PRIVATE -Wall -Wextra -Wfatal-errors)


add_executable(tests_integration test/integration.cpp)
target_link_libraries(tests_integration PRIVATE fmt::fmt)

target_link_libraries(tests_integration PRIVATE multidim)
