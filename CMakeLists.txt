cmake_minimum_required(VERSION 2.6)
project(theta)
set(CMAKE_VERBOSE_MAKEFILE, 1)

option(coverage "build for coverage tests" OFF)

if(coverage)
   message(STATUS "building for coverage test")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0")
endif(coverage)

find_package(Boost COMPONENTS regex program_options filesystem)

if(NOT Boost_FOUND)
   message(SEND_ERROR "boost not found")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(test_dir ${CMAKE_SOURCE_DIR}/test)

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(libconfig)
add_subdirectory(bin)
add_subdirectory(root)
add_subdirectory(test)
#add_subdirectory(test/test-stat)

