project(vrt_system_package CXX C)
cmake_minimum_required(VERSION 2.6.2)

include_directories(${vscript_SOURCE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

ADD_LIBRARY(vrt_system_package STATIC ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(vrt_system_package vscript)
