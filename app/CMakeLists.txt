if(CMAKE_SYSTEM_NAME STREQUAL 'Android')

else(CMAKE_SYSTEM_NAME STREQUAL 'Android')
add_subdirectory(vds_background)
add_subdirectory(vds_ws_server)
add_subdirectory(vds_get_root)
add_subdirectory(vds_cmd)
add_subdirectory(vds_updater)
endif(CMAKE_SYSTEM_NAME STREQUAL 'Android')

IF(MSVC)

	add_subdirectory(vds_windows)

ELSE(MSVC)

#	add_subdirectory(vds_linux)


ENDIF(MSVC)
