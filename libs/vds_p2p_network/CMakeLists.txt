project(vds_p2p_network CXX C)
cmake_minimum_required(VERSION 2.6.2)

find_package (OpenSSL REQUIRED)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB MESSAGES_HEADER_FILES messages/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_p2p_network STATIC ${SOURCE_LIB} ${PRIVATE_HEADER_FILES} ${MESSAGES_HEADER_FILES})

target_include_directories(vds_p2p_network
	PRIVATE
		 ${vds_core_INCLUDE_DIRS}
		 ${vds_crypto_INCLUDE_DIRS}
		 ${vds_network_INCLUDE_DIRS}
		 ${vds_parser_INCLUDE_DIRS}
		 ${vds_database_INCLUDE_DIRS}
		 ${vds_db_model_INCLUDE_DIRS}
		 ${vds_user_manager_INCLUDE_DIRS}
		 ${vds_chunk_manager_INCLUDE_DIRS}
		 ${vds_transactions_INCLUDE_DIRS}
		 ${OPENSSL_INCLUDE_DIR}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
        vds_p2p_network
        vds_core
        vds_crypto
        vds_network
        vds_parser
        vds_database
        vds_db_model
        vds_user_manager
        vds_chunk_manager
        vds_transactions
        ${OPENSSL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
)
