project(vds_protocols CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package (OpenSSL REQUIRED)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_protocols STATIC ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_protocols
		PRIVATE
			${vds_core_INCLUDE_DIRS}
			${vds_data_INCLUDE_DIRS}
			${vds_network_INCLUDE_DIRS}
			${vds_parser_INCLUDE_DIRS}
			${vds_crypto_INCLUDE_DIRS}
			${vds_database_INCLUDE_DIRS}
			${vds_p2p_network_INCLUDE_DIRS}
			${OPENSSL_INCLUDE_DIR}
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  vds_protocols
  vds_core
  vds_data
  vds_database
  vds_network
  vds_http
  vds_crypto
  vds_upnp
  vds_parser
  vds_p2p_network
  ${UUID_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${UPNP_LIBRARIES}
  ${PTHREAD_LIB}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})
