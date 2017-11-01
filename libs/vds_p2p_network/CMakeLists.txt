project(vds_p2p_network CXX C)
cmake_minimum_required(VERSION 2.6.2)

if (UNIX)
    find_package (OpenSSL REQUIRED)
else()
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
    find_package (OpenSSL REQUIRED)
endif (UNIX)


include_directories(${vds_core_SOURCE_DIR})
include_directories(${vds_crypto_SOURCE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

ADD_MSVC_PRECOMPILED_HEADER("stdafx.h" "stdafx.cpp" SOURCE_LIB)

add_library(vds_p2p_network STATIC ${SOURCE_LIB} ${HEADER_FILES} p2p_network_service.cpp p2p_network_service.h ip2p_network_storage.h private/udp_messages.h udp_transport.cpp udp_transport.h)
target_link_libraries(
        vds_p2p_network
        vds_core
        vds_crypto
        ${OPENSSL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
)
