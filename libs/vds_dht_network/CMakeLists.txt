project(vds_dht_network CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES include/*.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB MESSAGES_HEADER_FILES messages/*.h)
FILE(GLOB SOURCE_LIB impl/*.cpp)

SOURCE_GROUP(private FILES ${PRIVATE_HEADER_FILES})
SOURCE_GROUP(messages FILES ${MESSAGES_HEADER_FILES})
SOURCE_GROUP(include FILES ${HEADER_FILES})
SOURCE_GROUP(impl FILES ${SOURCE_LIB})

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "impl/stdafx.cpp" SOURCE_LIB)

add_library(vds_dht_network
		STATIC
			${SOURCE_LIB}
			${HEADER_FILES}
			${PRIVATE_HEADER_FILES}
			${MESSAGES_HEADER_FILES})

target_include_directories(vds_dht_network
	PRIVATE
		 ${vds_core_INCLUDE_DIRS}
		 ${vds_crypto_INCLUDE_DIRS}
		 ${vds_network_INCLUDE_DIRS}
		 ${vds_parser_INCLUDE_DIRS}
		 ${vds_data_INCLUDE_DIRS}
		 ${vds_database_INCLUDE_DIRS}
		 ${vds_db_model_INCLUDE_DIRS}
		 ${vds_user_manager_INCLUDE_DIRS}
         ${CMAKE_CURRENT_SOURCE_DIR}/private
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
		vds_dht_network
        vds_core
        vds_crypto
        vds_network
        vds_parser
		vds_data
        vds_database
        vds_db_model
        vds_user_manager
        ${CMAKE_THREAD_LIBS_INIT}
)
