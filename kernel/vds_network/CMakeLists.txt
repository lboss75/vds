project(vds_network CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_network STATIC ${PRIVATE_HEADER_FILES} ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_network
	PRIVATE
		${vds_core_INCLUDE_DIRS}
		${vds_parser_INCLUDE_DIRS}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vds_network vds_core vds_data vds_parser)
