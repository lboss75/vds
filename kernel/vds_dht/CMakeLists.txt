project(vds_dht CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_dht STATIC ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_dht
	PRIVATE
		${vds_core_INCLUDE_DIRS}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  vds_dht
  vds_core
  ${CMAKE_THREAD_LIBS_INIT})