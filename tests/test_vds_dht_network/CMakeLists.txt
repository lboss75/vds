cmake_minimum_required(VERSION 2.8)
project(test_vds_dht_network)

find_package (OpenSSL REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)
if(UNIX)
else(UNIX)
	FILE(GLOB GLOBAL_RES ${CMAKE_SOURCE_DIR}/resources/*.rc ${CMAKE_SOURCE_DIR}/resources/*.h)
endif(UNIX)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

add_executable(test_vds_dht_network ${SOURCE_FILES} ${HEADER_FILES} ${GLOBAL_RES})

target_include_directories(test_vds_dht_network
        PRIVATE
          ${vds_core_INCLUDE_DIRS}
          ${vds_network_INCLUDE_DIRS}
          ${vds_parser_INCLUDE_DIRS}
          ${vds_http_INCLUDE_DIRS}
          ${vds_crypto_INCLUDE_DIRS}
          ${vds_storage_INCLUDE_DIRS}
          ${vds_server_INCLUDE_DIRS}
          ${vds_dht_network_INCLUDE_DIRS}
          ${vds_user_manager_INCLUDE_DIRS}
          ${vds_transactions_INCLUDE_DIRS}
          ${test_libs_INCLUDE_DIRS}
          ${test_vds_servers_INCLUDE_DIRS}
          ${OPENSSL_INCLUDE_DIR})

enable_testing()

target_link_libraries(
  test_vds_dht_network
  vds_core
  vds_data
  vds_database
  vds_network
  vds_http
  vds_crypto
  vds_parser
  vds_server
  vds_user_manager
  vds_dht_network
  vds_transactions
  test_libs
  test_vds_servers
  ${OPENSSL_LIBRARIES}
  ${GTEST_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${EVENT_LIBRARY})

add_test(
    NAME runUnitTests
    COMMAND test_vds_dht_network
)
