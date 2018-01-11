project(vds_web_server CXX)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

add_executable(vds_web_server ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(vds_web_server
        PRIVATE
	        ${vds_core_INCLUDE_DIRS}
        	${vds_crypto_INCLUDE_DIRS}
	        ${vds_data_INCLUDE_DIRS}
	        ${vds_database_INCLUDE_DIRS}
	        ${vds_db_model_INCLUDE_DIRS}
	        ${vds_p2p_network_INCLUDE_DIRS}
			${vds_network_INCLUDE_DIRS}
			${vds_parser_INCLUDE_DIRS}
			${vds_http_INCLUDE_DIRS}
			${vds_protocols_INCLUDE_DIRS}
			${vds_storage_INCLUDE_DIRS}
			${vds_client_INCLUDE_DIRS}
			${vds_server_INCLUDE_DIRS}
			${vds_user_manager_INCLUDE_DIRS}
			${vds_transactions_INCLUDE_DIRS}
        PUBLIC
        	${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vds_web_server
        vds_chunk_manager
        vds_core
        vds_crypto
        vds_data
        vds_database
        vds_db_model
        vds_p2p_network
	vds_network
	vds_parser
	vds_http
	vds_protocols
	vds_storage
	vds_client
	vds_server
	vds_user_manager
	vds_transactions)
