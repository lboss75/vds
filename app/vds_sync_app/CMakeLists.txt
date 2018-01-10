project(vds_sync_app CXX)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

add_library(vds_sync_app STATIC ${SOURCE_LIB} ${HEADER_FILES})

target_include_directories(
        vds_sync_app
        PRIVATE
        ${vds_core_INCLUDE_DIRS}
        ${vds_crypto_INCLUDE_DIRS}
        ${vds_data_INCLUDE_DIRS}
        ${vds_database_INCLUDE_DIRS}
        ${vds_db_model_INCLUDE_DIRS}
        ${vds_p2p_network_INCLUDE_DIRS}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vds_sync_app
        vds_chunk_manager
        vds_core
        vds_crypto
        vds_data
        vds_database
        vds_db_model
        vds_p2p_network)