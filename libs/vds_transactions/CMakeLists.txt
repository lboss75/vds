project(vds_transactions CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package (OpenSSL REQUIRED)

FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB TRANSACTIONS_HEADER_FILES transactions/*.h)
FILE(GLOB SOURCE_LIB *.cpp)
FILE(GLOB HEADER_FILES include/*.h)

ADD_MSVC_PRECOMPILED_HEADER("private/stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_transactions
		STATIC
			${PRIVATE_HEADER_FILES}
			${SOURCE_LIB}
			${HEADER_FILES}
			${TRANSACTIONS_HEADER_FILES})

target_include_directories(vds_transactions
	PRIVATE
		${vds_core_INCLUDE_DIRS}
		${vds_network_INCLUDE_DIRS}
		${vds_crypto_INCLUDE_DIRS}
		${vds_data_INCLUDE_DIRS}
		${vds_http_INCLUDE_DIRS}
		${vds_parser_INCLUDE_DIRS}
		${vds_database_INCLUDE_DIRS}
		${vds_db_model_INCLUDE_DIRS}
		${vds_user_manager_INCLUDE_DIRS}
		${vds_dht_INCLUDE_DIRS}
		${vds_dht_network_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vds_transactions
  vds_core
  vds_data
  vds_database
  vds_db_model
  vds_network
  vds_http
  vds_crypto
  vds_upnp
  vds_parser
  vds_user_manager
  vds_dht
  vds_dht_network
  ${UUID_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${UPNP_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})
