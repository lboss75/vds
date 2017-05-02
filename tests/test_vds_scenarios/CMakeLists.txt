cmake_minimum_required(VERSION 2.8)
project(test_vds_scenarios)

if (UNIX)
	find_package (OpenSSL REQUIRED)
	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
	find_package (libevent REQUIRED)
	find_package (UUID REQUIRED)
else()
	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
	find_package (OpenSSL REQUIRED)
endif (UNIX)

find_package (UPNP REQUIRED)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_network_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})
include_directories(${vds_http_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${vds_upnp_SOURCE_DIR})
include_directories(${vds_protocols_SOURCE_DIR})
include_directories(${vds_storage_SOURCE_DIR})
include_directories(${vds_client_SOURCE_DIR})
include_directories(${vds_server_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${UPNP_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

enable_testing()

add_executable(test_vds_scenarios ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(
  test_vds_scenarios
  vds_core
  vds_data
  vds_database
  vds_network
  vds_http
  vds_crypto
  vds_upnp
  vds_parser
  vds_protocols
  vds_client
  vds_server
  ${UUID_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${UPNP_LIBRARIES}
  ${GTEST_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${EVENT_LIBRARY})

add_test(
    NAME runUnitTests
    COMMAND test_vds_scenarios
)
