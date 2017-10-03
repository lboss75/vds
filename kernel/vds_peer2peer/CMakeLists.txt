project(vds_peer2peer CXX C)
cmake_minimum_required(VERSION 2.6.2)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})
include_directories(${vds_network_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_peer2peer STATIC ${SOURCE_LIB} ${HEADER_FILES})

target_link_libraries(vds_peer2peer vds_core vds_data vds_parser vds_network vds_crypto)
