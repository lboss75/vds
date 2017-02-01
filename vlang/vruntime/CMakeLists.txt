project(vruntime CXX C)
cmake_minimum_required(VERSION 2.6.2)

include_directories(${vscript_SOURCE_DIR})
include_directories(${vds_core_SOURCE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)
ADD_LIBRARY(vruntime STATIC ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(vruntime vscript vds_core)
