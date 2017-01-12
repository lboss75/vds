find_package (GTest REQUIRED)
find_package (Threads REQUIRED)

include_directories(
    ${GTEST_INCLUDE_DIRS}
)

add_subdirectory(test_vds_core)
