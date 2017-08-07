project(vds_network CXX C)
cmake_minimum_required(VERSION 2.6.2)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_parser_SOURCE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)
add_library(vds_network STATIC ${SOURCE_LIB} ${HEADER_FILES})

if (UNIX)
target_link_libraries(vds_network vds_core vds_data vds_parser ${EVENT_LIBRARY})
else()
target_link_libraries(vds_network vds_core vds_data vds_parser)
endif (UNIX)