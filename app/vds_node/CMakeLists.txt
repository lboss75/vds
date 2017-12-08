project(vds_node CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package (OpenSSL REQUIRED)

if (UNIX)
  set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
  find_package (UUID REQUIRED)
endif (UNIX)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})
include_directories(${vds_http_SOURCE_DIR})
include_directories(${vds_network_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${vds_protocols_SOURCE_DIR})
include_directories(${vds_client_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)
add_executable(vds_node ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(
  vds_node
  vds_core
  vds_data
  vds_database
  vds_crypto
  vds_network
  vds_http
  vds_parser
  vds_protocols
  vds_client
  ${UUID_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})


