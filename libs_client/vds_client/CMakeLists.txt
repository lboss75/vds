project(vds_client CXX)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_client 
	STATIC
		${PRIVATE_HEADER_FILES}
		${SOURCE_LIB}
		${HEADER_FILES})

target_include_directories(vds_client
	PRIVATE
		${vds_core_INCLUDE_DIRS}
		${vds_crypto_INCLUDE_DIRS}
		${vds_transactions_INCLUDE_DIRS}
		${vds_http_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vds_client
  vds_core
  vds_crypto
  vds_transactions
  vds_http
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )
