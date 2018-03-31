project(test_libs CXX)
cmake_minimum_required(VERSION 2.8)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

add_library(test_libs
        STATIC
            ${SOURCE_FILES}
            ${HEADER_FILES})

target_include_directories(test_libs
        PRIVATE
            ${vds_core_INCLUDE_DIRS}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(test_libs
        vds_core)