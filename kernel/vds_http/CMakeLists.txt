project(vds_http CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package (OpenSSL REQUIRED)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)
add_library(vds_http STATIC ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_http
	PRIVATE
		${vds_core_INCLUDE_DIRS}
		${vds_network_INCLUDE_DIRS}
		${vds_parser_INCLUDE_DIRS}
		${vds_crypto_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(
  vds_http
  vds_core
  vds_crypto
  vds_network
  ${OPENSSL_LIBRARIES})