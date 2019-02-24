if(CMAKE_SYSTEM_NAME STREQUAL 'Android')

else(CMAKE_SYSTEM_NAME STREQUAL 'Android')
add_subdirectory(vds_background)
add_subdirectory(vds_web_server)
add_subdirectory(vds_get_root)
add_subdirectory(vds_cmd)
endif(CMAKE_SYSTEM_NAME STREQUAL 'Android')

add_subdirectory(vds_embedded)

IF(MSVC)

INCLUDE_EXTERNAL_MSPROJECT(
	vds_wf_client
	${CMAKE_CURRENT_LIST_DIR}/vds_wf_client/vds_wf_client.csproj
)

ELSE(MSVC)

	add_subdirectory(vds_linux)


ENDIF(MSVC)
