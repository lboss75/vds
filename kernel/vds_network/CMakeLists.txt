project(vds_network CXX C)
cmake_minimum_required(VERSION 2.6.2)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_network)
target_sources(vds_network
        PRIVATE
            ${PRIVATE_HEADER_FILES}
            ${SOURCE_LIB}
        PUBLIC
            ${HEADER_FILES})

if (UNIX)
target_link_libraries(vds_network vds_core vds_data vds_parser ${EVENT_LIBRARY})
else()
target_link_libraries(vds_network vds_core vds_data vds_parser)
endif (UNIX)