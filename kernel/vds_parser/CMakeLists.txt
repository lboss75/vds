project(vds_parser CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

include_directories(${vds_core_SOURCE_DIR})

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

ADD_LIBRARY(vds_parser STATIC ${SOURCE_LIB} ${HEADER_FILES})
target_link_libraries(vds_parser vds_core)
