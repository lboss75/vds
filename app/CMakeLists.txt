if(CMAKE_SYSTEM_NAME STREQUAL 'Android')

else(CMAKE_SYSTEM_NAME STREQUAL 'Android')
add_subdirectory(vds_background)
add_subdirectory(vds_web_server)
add_subdirectory(vds_get_root)
add_subdirectory(vds_cmd)
add_subdirectory(vds_updater)
endif(CMAKE_SYSTEM_NAME STREQUAL 'Android')

add_subdirectory(vds_embedded)

IF(MSVC)

	add_subdirectory(vds_windows)

ELSE(MSVC)

#	add_subdirectory(vds_linux)


ENDIF(MSVC)
