project(vds_data CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package( ZLIB REQUIRED )

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_data STATIC ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_data
	PRIVATE
		${vds_core_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  vds_data
  vds_core
  ${ZLIB_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})