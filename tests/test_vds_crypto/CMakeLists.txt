cmake_minimum_required(VERSION 2.8)
project(test_vds_crypto)

if (UNIX)
	find_package (OpenSSL REQUIRED)
else()
	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
	find_package (OpenSSL REQUIRED)
endif (UNIX)

enable_testing()
FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

add_executable(test_vds_crypto ${SOURCE_FILES} ${HEADER_FILES})

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

target_link_libraries(test_vds_crypto vds_core vds_crypto ${GTEST_LIBRARIES} ${OPENSSL_LIBRARIES})

add_test(
    NAME runUnitTests
    COMMAND test_vds_crypto
)
