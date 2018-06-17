project(vds_embedded CXX C)
cmake_minimum_required(VERSION 2.6.2)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

ADD_LIBRARY(vds_embedded SHARED ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(
	vds_embedded
	PRIVATE
		${vds_core_INCLUDE_DIR}
		${vds_network_INCLUDE_DIR}
		${vds_parser_INCLUDE_DIR}
		${vds_crypto_INCLUDE_DIR}
		${vds_server_INCLUDE_DIR}
		${vds_dht_network_INCLUDE_DIR}
		${vds_user_manager_INCLUDE_DIR}
		${vds_transactions_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  vds_embedded
  vds_core
  vds_data
  vds_database
  vds_network
  vds_crypto
  vds_parser
  vds_server
  vds_dht_network
  vds_user_manager
  vds_transactions
  ${UUID_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${PTHREAD_LIB}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})
