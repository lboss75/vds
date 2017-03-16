project(vds_data CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package( ZLIB REQUIRED )

if (UNIX)
	find_package (OpenSSL REQUIRED)
else()
	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
	find_package (OpenSSL REQUIRED)
endif (UNIX)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS} )
include_directories(${OPENSSL_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)
add_library(vds_data STATIC ${SOURCE_LIB} ${HEADER_FILES})
target_link_libraries(
  vds_data
  vds_core
  vds_crypto
  ${ZLIB_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})