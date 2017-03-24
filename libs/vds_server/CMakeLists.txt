project(vds_server CXX C)
cmake_minimum_required(VERSION 2.6.2)

if (UNIX)
	find_package (OpenSSL REQUIRED)
else()
	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
	find_package (OpenSSL REQUIRED)
endif (UNIX)


include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_network_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${vds_data_SOURCE_DIR})
include_directories(${vds_http_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})
include_directories(${vds_protocols_SOURCE_DIR})
include_directories(${vds_storage_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_server STATIC ${SOURCE_LIB} ${HEADER_FILES})
target_link_libraries(
  vds_server
  vds_core
  vds_data
  vds_network
  vds_http
  vds_crypto
  vds_upnp
  vds_parser
  vds_protocols
  vds_storage
  ${UUID_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${UPNP_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})
