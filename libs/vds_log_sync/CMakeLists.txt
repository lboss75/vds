project(vds_log_sync CXX C)
cmake_minimum_required(VERSION 2.6.2)

include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${vds_database_SOURCE_DIR})
include_directories(${vds_db_model_SOURCE_DIR})
include_directories(${vds_transactions_SOURCE_DIR})
include_directories(${vds_p2p_network_SOURCE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_log_sync)
target_sources(
		vds_log_sync
		PRIVATE
			${PRIVATE_HEADER_FILES}
			${SOURCE_LIB}
		PUBLIC
			${HEADER_FILES})

target_link_libraries(
  vds_log_sync
  vds_core
  vds_crypto
  vds_database
  vds_db_model
  vds_transactions
  vds_p2p_network
  ${CMAKE_THREAD_LIBS_INIT}
)
