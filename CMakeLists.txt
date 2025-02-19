cmake_minimum_required(VERSION 3.10)
project(HelloWorld)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(HelloWorld src/helloworld.cpp)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(HelloWorld PRIVATE fmt::fmt)
target_include_directories(HelloWorld PRIVATE "deps/gch")
target_compile_options(HelloWorld PRIVATE -Wall -Wextra)
