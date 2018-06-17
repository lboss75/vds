cmake_minimum_required(VERSION 2.8)
project(test_vds_dht_network)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

add_executable(test_vds_dht_network ${SOURCE_FILES} ${HEADER_FILES})

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
          ${test_libs_INCLUDE_DIRS})

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
  ${GTEST_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${EVENT_LIBRARY})

add_test(
    NAME runUnitTests
    COMMAND test_vds_dht_network
)
