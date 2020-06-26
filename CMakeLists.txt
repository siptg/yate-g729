cmake_minimum_required(VERSION 3.1)
project(g729bc)

set(CMAKE_CXX_STANDARD 14)

set(ENABLE_SHARED NO)
set(ENABLE_STATIC YES)
set(ENABLE_TESTS NO)
add_subdirectory(bcg729)

include_directories(yate bcg729/include)

add_library(g729bc MODULE library.cpp library.h)
set_target_properties(g729bc PROPERTIES PREFIX "" SUFFIX ".yate")
target_link_libraries(g729bc bcg729-static)
