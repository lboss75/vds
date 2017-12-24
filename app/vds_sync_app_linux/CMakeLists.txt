project(vds_sync_app_linux)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)

# Now the variables GTKMM_INCLUDE_DIRS, GTKMM_LIBRARY_DIRS and GTKMM_LIBRARIES
# contain what you expect

link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS})

FILE(GLOB HEADER_FILES *.h)
FILE(GLOB SOURCE_LIB *.cpp)

add_executable(vds_sync_app_linux ${SOURCE_LIB} ${HEADER_FILES})

target_link_libraries(vds_sync_app_linux
        ${GTKMM_LIBRARIES})