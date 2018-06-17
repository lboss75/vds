project(vds_web_server CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_executable(vds_web_server ${PRIVATE_HEADER_FILES} ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_web_server
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
		${vds_file_manager_INCLUDE_DIRS}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  vds_web_server
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
  vds_file_manager
  vds_server
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})
