project(vds_background CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package (OpenSSL REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

if (UNIX)
  find_package (UUID REQUIRED)
endif (UNIX)

find_package (UPNP REQUIRED)


include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_network_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})
include_directories(${vds_http_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${vds_upnp_SOURCE_DIR})
include_directories(${vds_protocols_SOURCE_DIR})
include_directories(${vds_server_SOURCE_DIR})
include_directories(${vds_p2p_network_SOURCE_DIR})
include_directories(${vds_user_manager_SOURCE_DIR})
include_directories(${vds_transactions_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${UPNP_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

add_executable(vds_background ${SOURCE_FILES} ${HEADER_FILES})
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
  ${UUID_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${UPNP_LIBRARIES}
  ${PTHREAD_LIB}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})
