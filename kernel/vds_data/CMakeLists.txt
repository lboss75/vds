project(vds_data CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package( ZLIB REQUIRED )

include_directories(${vds_core_SOURCE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS} )

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)
add_library(vds_data STATIC ${SOURCE_LIB} ${HEADER_FILES})
target_link_libraries(
  vds_data
  vds_core
  ${ZLIB_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})