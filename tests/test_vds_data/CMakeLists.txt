cmake_minimum_required(VERSION 2.8)
project(test_vds_data)
enable_testing()

find_package( ZLIB REQUIRED )

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)
ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)


include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_data_SOURCE_DIR})
include_directories(${test_libs_INCLUDE_DIRS})
include_directories(${ZLIB_INCLUDE_DIRS} )

add_executable(test_vds_data ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(
  test_vds_data
  vds_core
  vds_data
  test_libs
  ${GTEST_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${ZLIB_LIBRARIES})

add_test(
    NAME runUnitTests
    COMMAND test_vds_data
)
