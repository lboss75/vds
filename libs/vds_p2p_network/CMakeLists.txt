project(vds_p2p_network CXX C)
cmake_minimum_required(VERSION 2.6.2)

if (UNIX)
    find_package (OpenSSL REQUIRED)
else()
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
    find_package (OpenSSL REQUIRED)
endif (UNIX)


include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${vds_network_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_p2p_network)

target_sources(vds_p2p_network
        PRIVATE
            ${PRIVATE_HEADER_FILES}
            ${SOURCE_LIB}
        PUBLIC
            ${HEADER_FILES})

target_link_libraries(
        vds_p2p_network
        vds_core
        vds_crypto
        vds_network
        vds_parser
        ${OPENSSL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
)
