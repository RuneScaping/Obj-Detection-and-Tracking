cmake_minimum_required(VERSION 2.8)

project(detect)

SET(CMAKE_C_COMPILER g++)
if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(-std=c++11)
message(STATUS "optional:-std=c++11")
endif(CMAKE_COMPILER_IS_GNUCXX)

find_package(OpenCV REQUIRED)

message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VE