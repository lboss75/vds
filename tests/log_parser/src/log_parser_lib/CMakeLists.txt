project(log_parser_lib CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "impl/stdafx.cpp" SOURCE_LIB)

add_library(log_parser_lib
		STATIC
			${SOURCE_LIB}
			${HEADER_FILES})

target_include_directories(log_parser_lib
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
		log_parser_lib
)
