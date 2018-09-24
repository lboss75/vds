if(ANDROID_PLATFORM)
else(ANDROID_PLATFORM)
add_subdirectory(vds_background)
add_subdirectory(vds_web_server)
add_subdirectory(vds_get_root)
endif(ANDROID_PLATFORM)

add_subdirectory(vds_embedded)
add_subdirectory(test)