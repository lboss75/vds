project(vds_database CXX C)
cmake_minimum_required(VERSION 2.6.2)

include_directories(${vds_core_SOURCE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_database
  STATIC
  ${SOURCE_LIB}
  ${HEADER_FILES}
  private/sqllite3/shell.c
  private/sqllite3/sqlite3.c
  private/sqllite3/sqlite3.h
  private/sqllite3/sqlite3ext.h)

target_link_libraries(
  vds_database
  vds_core
  ${PTHREAD_LIB}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})
