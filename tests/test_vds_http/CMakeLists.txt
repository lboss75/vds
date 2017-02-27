cmake_minimum_required(VERSION 2.8)
project(test_vds_http)

if (UNIX)
  find_package (OpenSSL REQUIRED)
  set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
	find_package (libevent REQUIRED)
else(UNIX)
  set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
  find_package (OpenSSL REQUIRED)
endif (UNIX)

enable_testing()
FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)
add_executable(test_vds_http ${SOURCE_FILES} ${HEADER_FILES})
include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_network_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})
include_directories(${vds_http_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(
  test_vds_http
  vds_core
  vds_network
  vds_parser
  vds_crypto
  vds_http
  ${GTEST_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${EVENT_LIBRARY})
  
add_test(
    NAME runUnitTests
    COMMAND test_vds_http
)
