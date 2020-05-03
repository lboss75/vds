project(vds_ws_server CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

if(UNIX)
	find_package (Threads REQUIRED)
else(UNIX)
	FILE(GLOB GLOBAL_RES ${CMAKE_SOURCE_DIR}/resources/*.rc ${CMAKE_SOURCE_DIR}/resources/*.h)
endif(UNIX)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_executable(vds_ws_server
	 ${PRIVATE_HEADER_FILES}
	 ${SOURCE_LIB}
	 ${HEADER_FILES}
	 ${GLOBAL_RES})

target_include_directories(vds_ws_server
	PRIVATE
		${vds_core_INCLUDE_DIRS}
		${vds_network_INCLUDE_DIRS}
		${vds_crypto_INCLUDE_DIRS}
		${vds_data_INCLUDE_DIRS}
		${vds_http_INCLUDE_DIRS}
		${vds_parser_INCLUDE_DIRS}
		${vds_database_INCLUDE_DIRS}
		${vds_transactions_INCLUDE_DIRS}
		${vds_user_manager_INCLUDE_DIRS}
		${vds_db_model_INCLUDE_DIRS}
		${vds_server_INCLUDE_DIR}
		${vds_dht_network_INCLUDE_DIRS}
		${vds_ws_api_INCLUDE_DIRS}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vds_ws_server
  vds_core
  vds_data
  vds_database
  vds_network
  vds_http
  vds_crypto
  vds_parser
  vds_transactions
  vds_user_manager
  vds_dht_network
  vds_db_model
  vds_server
  vds_ws_api
  OpenSSL::Crypto
  ${CMAKE_THREAD_LIBS_INIT})
