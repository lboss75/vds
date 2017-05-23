cmake_minimum_required(VERSION 2.8)
project(test_libs)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

include_directories(${vds_core_SOURCE_DIR})

ADD_LIBRARY(test_libs STATIC ${SOURCE_FILES} ${HEADER_FILES})
