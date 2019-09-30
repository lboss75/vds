project(vds_user_manager CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_user_manager 
	STATIC
		${PRIVATE_HEADER_FILES}
		${SOURCE_LIB}
		${HEADER_FILES})

target_include_directories(vds_user_manager
	PRIVATE
		${vds_core_INCLUDE_DIRS}
		${vds_crypto_INCLUDE_DIRS}
		${vds_database_INCLUDE_DIRS}
		${vds_db_model_INCLUDE_DIRS}
		${vds_dht_INCLUDE_DIRS}
		${vds_transactions_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  vds_user_manager
  vds_core
  vds_crypto
  vds_database
  vds_db_model
  vds_transactions
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )
