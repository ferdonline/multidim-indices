cmake_minimum_required(VERSION 3.10)
project(MultiDim)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MULTIDIM_BENCHMARKING "Enable benchmarking" TRUE)
option(MULTIDIM_DEBUG "Print intermediate debug data using fmt")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

include_directories(SYSTEM "external")
include_directories("include")
add_compile_options(-Wall -Wextra -Wfatal-errors -march=native -fopt-info-vec-missed)

if(MULTIDIM_DEBUG)
  find_package(fmt CONFIG REQUIRED)
  add_compile_definitions(MULTIDIM_DEBUG=1)
  link_libraries(fmt::fmt)
endif()

add_library(multidim OBJECT src/multidim.cpp)

add_executable(tests_unit test/unit.cpp)
target_link_libraries(tests_unit multidim)

if (MULTIDIM_BENCHMARKING)
  # Dont reuse multidim.o. Needs different compile option to not append all values to output array
  add_executable(benchmark src/multidim.cpp test/benchmark.cpp)
  target_compile_definitions(benchmark PRIVATE MULTIDIM_BENCHMARKING=1)
endif()
