project(vds_network CXX C)
cmake_minimum_required(VERSION 2.6.2)

if (UNIX)
  set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
  find_package (libevent REQUIRED)
endif (UNIX)

include_directories(${vds_core_SOURCE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)
add_library(vds_network STATIC ${SOURCE_LIB} ${HEADER_FILES})

if (UNIX)
target_link_libraries(vds_network vds_core ${EVENT_LIBRARY})
else()
target_link_libraries(vds_network vds_core)
endif (UNIX)