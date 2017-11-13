project(vds_db_model CXX C)
cmake_minimum_required(VERSION 2.6.2)

if (UNIX)
    find_package (OpenSSL REQUIRED)
else()
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
    find_package (OpenSSL REQUIRED)
endif (UNIX)


include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_database_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB PRIVATE_HEADER_FILES private/*.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_db_model)

target_sources(vds_db_model
        PRIVATE
            ${PRIVATE_HEADER_FILES}
            ${SOURCE_LIB}
        PUBLIC
            ${HEADER_FILES})

target_link_libraries(
        vds_db_model
        vds_core
        vds_database
        ${OPENSSL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
)
