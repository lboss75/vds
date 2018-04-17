cmake_minimum_required(VERSION 2.8)
project(log_parser_test)

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_FILES *.cpp)

add_executable(log_parser_test ${SOURCE_FILES} ${HEADER_FILES})

enable_testing()

target_link_libraries(
        log_parser_test
        log_parser_lib
  ${GTEST_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})

add_test(
    NAME runUnitTests
    COMMAND log_parser_test
)
