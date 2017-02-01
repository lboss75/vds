project(vscript CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)
add_library(vscript STATIC ${SOURCE_LIB} ${HEADER_FILES})
