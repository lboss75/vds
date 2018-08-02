project(vds_get_root CXX C)
cmake_minimum_required(VERSION 2.6.2)

if(UNIX)
	find_package (Threads REQUIRED)
endif(UNIX)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_FILES)

add_executable(vds_get_root ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(vds_get_root
	PRIVATE
		${vds_core_INCLUDE_DIR}
		${vds_network_INCLUDE_DIR}
		${vds_crypto_INCLUDE_DIR}
		${vds_user_manager_INCLUDE_DIR}
		${UPNP_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(
  vds_get_root
  vds_core
  vds_crypto
  vds_network
  vds_user_manager
  ${PTHREAD_LIB}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})
