cmake_minimum_required(VERSION 3.8)

project(ujson)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSONS OFF)

add_library(ujson INTERFACE
  include/ujson/ujson.h
)

target_include_directories(ujson INTERFACE
  include
)
