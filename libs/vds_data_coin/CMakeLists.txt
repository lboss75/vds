project(vds_data_coin CXX C)
cmake_minimum_required(VERSION 2.6.2)

FILE(GLOB HEADER_FILES include/*.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_FILES impl/*.cpp)
FILE(GLOB MESSAGES_FILES messages/*.h)
FILE(GLOB ORM_FILES orm/*.h)
FILE(GLOB TRANSACTIONS_FILES transactions/*.h)

SOURCE_GROUP(private FILES ${PRIVATE_HEADER_FILES})
SOURCE_GROUP(include FILES ${HEADER_FILES})
SOURCE_GROUP(impl FILES ${SOURCE_FILES})
SOURCE_GROUP(messages FILES ${MESSAGES_FILES})
SOURCE_GROUP(orm FILES ${ORM_FILES})
SOURCE_GROUP(transactions FILES ${TRANSACTIONS_FILES})

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "impl/stdafx.cpp" SOURCE_FILES)

add_library(vds_data_coin
		STATIC
			${SOURCE_FILES}
			${HEADER_FILES}
			${PRIVATE_HEADER_FILES}
			${MESSAGES_FILES}
            ${ORM_FILES}
		    ${TRANSACTIONS_FILES})

target_include_directories(vds_data_coin
	PRIVATE
		 ${vds_core_INCLUDE_DIRS}
		 ${vds_crypto_INCLUDE_DIRS}
		 ${vds_parser_INCLUDE_DIRS}
		 ${vds_data_INCLUDE_DIRS}
		 ${vds_database_INCLUDE_DIRS}
		 ${vds_db_model_INCLUDE_DIRS}
		 ${OPENSSL_INCLUDE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/private
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
		vds_data_coin
        vds_core
        vds_crypto
        vds_parser
		vds_data
        vds_database
        vds_db_model
        ${OPENSSL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
)
