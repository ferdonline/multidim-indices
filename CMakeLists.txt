cmake_minimum_required(VERSION 3.10)
project(HelloWorld)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MULTIDIM_DEBUG "Print intermediate debug data using fmt")

include_directories(SYSTEM "external")
include_directories("include")
if(MULTIDIM_DEBUG)
    find_package(fmt CONFIG REQUIRED)
    add_compile_definitions(MULTIDIM_DEBUG=1)
    link_libraries(fmt::fmt)
endif()

add_library(multidim src/multidim.cpp)
target_compile_options(multidim PRIVATE -Wall -Wextra -Wfatal-errors)
# target_compile_options(multidim PUBLIC "-mavx2")

add_executable(tests_unit test/unit.cpp)
target_link_libraries(tests_unit PRIVATE multidim)
