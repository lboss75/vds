project(vds_core CXX C)
cmake_minimum_required(VERSION 2.6.2)

if (UNIX)
  set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
  find_package (UUID REQUIRED)
endif (UNIX)


FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

ADD_LIBRARY(vds_core STATIC ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
 
target_link_libraries(
  vds_core
  ${UUID_LIBRARIES})