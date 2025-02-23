cmake_minimum_required(VERSION 3.10)
project(MultiDim)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

include_directories(SYSTEM "external")
include_directories("include")
add_compile_options(-Wall -Wextra -Wfatal-errors -march=native)

option(MULTIDIM_DEBUG "Print intermediate debug data using fmt")
if(MULTIDIM_DEBUG)
    find_package(fmt CONFIG REQUIRED)
    add_compile_definitions(MULTIDIM_DEBUG=1)
    link_libraries(fmt::fmt)
endif()

add_library(multidim OBJECT src/multidim.cpp)

add_executable(tests_unit test/unit.cpp)
target_link_libraries(tests_unit multidim)

add_executable(benchmark test/benchmark.cpp)
target_link_libraries(benchmark multidim)
