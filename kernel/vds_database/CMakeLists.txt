project(vds_database CXX C)
cmake_minimum_required(VERSION 2.6.2)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${SQLITE3_INCLUDE_DIRS} )

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_database
  STATIC
  ${SOURCE_LIB}
  ${HEADER_FILES}
  sqllite3/shell.c
  sqllite3/sqlite3.c
  sqllite3/sqlite3.h
  sqllite3/sqlite3ext.h)

target_link_libraries(
  vds_database
  vds_core
  ${CMAKE_THREAD_LIBS_INIT})