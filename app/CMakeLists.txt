if(ANDROID_PLATFORM)
else(ANDROID_PLATFORM)
add_subdirectory(vds_background)
add_subdirectory(vds_web_server)
endif(ANDROID_PLATFORM)

add_subdirectory(vds_embedded)