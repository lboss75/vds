project(vds_file_manager CXX)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB TRANSACTIONS_HEADER_FILES transactions/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_file_manager STATIC
            ${PRIVATE_HEADER_FILES}
            ${SOURCE_LIB}
            ${HEADER_FILES}
            ${TRANSACTIONS_HEADER_FILES})

target_include_directories(vds_file_manager
	PRIVATE
		${vds_core_INCLUDE_DIRS}
		${vds_db_model_INCLUDE_DIRS}
        ${vds_transactions_INCLUDE_DIRS}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
		vds_file_manager
        vds_core
        vds_db_model
        vds_transactions
        ${CMAKE_THREAD_LIBS_INIT})
