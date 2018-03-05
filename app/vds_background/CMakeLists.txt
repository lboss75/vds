project(vds_background CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package (OpenSSL REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

if (UNIX)
  find_package (UUID REQUIRED)
endif (UNIX)

find_package (UPNP REQUIRED)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

add_executable(vds_background ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(vds_background
	PRIVATE
		${vds_core_INCLUDE_DIR}
		${vds_network_INCLUDE_DIR}
		${vds_parser_INCLUDE_DIR}
		${vds_http_INCLUDE_DIR}
		${vds_crypto_INCLUDE_DIR}
		${vds_upnp_INCLUDE_DIR}
		${vds_protocols_INCLUDE_DIR}
		${vds_server_INCLUDE_DIR}
		${vds_p2p_network_INCLUDE_DIR}
		${vds_user_manager_INCLUDE_DIR}
		${vds_transactions_INCLUDE_DIR}
		${vds_web_server_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
		${UPNP_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(
  vds_background
  vds_core
  vds_data
  vds_database
  vds_network
  vds_http
  vds_crypto
  vds_upnp
  vds_parser
  vds_protocols
  vds_server
  vds_p2p_network
  vds_user_manager
  vds_transactions
  vds_web_server
  ${UUID_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${UPNP_LIBRARIES}
  ${PTHREAD_LIB}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})
