cmake_minimum_required(VERSION 2.6)
project(theta)
set(CMAKE_VERBOSE_MAKEFILE, 1)

option(coverage "build for coverage tests" OFF)
option(release "build release version" ON)
option(crlibm "use the log function from crlibm" ON)
option(openmp "enable parallelization" ON)
option(psql "use postgresql" OFF)

if(coverage)
   message(STATUS "building for coverage test")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0")
else()
  if(release)
     set(CMAKE_BUILD_TYPE Release)
     message(STATUS "building release version")
  else()
     message(STATUS "building release version +debug")
     set(CMAKE_BUILD_TYPE RelWithDebInfo)
   endif()
endif()

if(crlibm)
   message(STATUS "using log from crlibm")
   add_definitions(-DUSE_CRLIBM)
endif()

if(openmp)
   message(STATUS "enabling parallelization")
   add_definitions(-DUSE_OPENMP)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -pthread")
endif()


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
