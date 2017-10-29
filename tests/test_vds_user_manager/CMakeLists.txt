cmake_minimum_required(VERSION 2.8)
project(test_vds_user_manager)

if (UNIX)
	find_package (OpenSSL REQUIRED)
	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
else()
	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
	find_package (OpenSSL REQUIRED)
endif (UNIX)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${vds_user_manager_SOURCE_DIR})
include_directories(${test_libs_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

enable_testing()

add_executable(test_vds_user_manager ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(
  test_vds_user_manager
  vds_core
  vds_crypto
  vds_user_manager
  ${OPENSSL_LIBRARIES}
  ${GTEST_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})

add_test(
    NAME runUnitTests
    COMMAND test_vds_user_manager
)
