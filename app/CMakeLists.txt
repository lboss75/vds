if(CMAKE_SYSTEM_NAME STREQUAL 'Android')

else(CMAKE_SYSTEM_NAME STREQUAL 'Android')
add_subdirectory(vds_background)
add_subdirectory(vds_web_server)
add_subdirectory(vds_get_root)
add_subdirectory(vds_cmd)
endif(CMAKE_SYSTEM_NAME STREQUAL 'Android')

add_subdirectory(vds_embedded)
