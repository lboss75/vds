project(vds_updater CXX C)
cmake_minimum_required(VERSION 2.6.2)

if(UNIX)
	find_package (Threads REQUIRED)
endif(UNIX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

if(UNIX)
else(UNIX)
	FILE(GLOB GLOBAL_RES ${CMAKE_SOURCE_DIR}/resources/*.rc ${CMAKE_SOURCE_DIR}/resources/*.h)
endif(UNIX)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

add_executable(vds_updater ${SOURCE_FILES} ${HEADER_FILES} ${GLOBAL_RES})

target_include_directories(vds_updater
	PRIVATE
		${vds_core_INCLUDE_DIR}
		${UPNP_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  vds_updater
  vds_core
  ${PTHREAD_LIB}
  ${CMAKE_THREAD_LIBS_INIT})
