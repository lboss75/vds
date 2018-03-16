project(vds_log_sync CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_log_sync STATIC ${PRIVATE_HEADER_FILES} ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_log_sync
    PRIVATE
        ${vds_core_INCLUDE_DIRS}
        ${vds_crypto_INCLUDE_DIRS}
        ${vds_database_INCLUDE_DIRS}
        ${vds_db_model_INCLUDE_DIRS}
        ${vds_transactions_INCLUDE_DIRS}
        ${vds_dht_network_INCLUDE_DIRS}
		${vds_chunk_manager_INCLUDE_DIRS}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  vds_log_sync
  vds_core
  vds_crypto
  vds_database
  vds_db_model
  vds_transactions
  vds_dht_network
  vds_chunk_manager
  ${CMAKE_THREAD_LIBS_INIT}
)
