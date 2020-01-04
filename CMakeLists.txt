cmake_minimum_required(VERSION 2.6.2)

project(vds)

include(CMakeLists.inc)

add_subdirectory(kernel)
add_subdirectory(libs)
add_subdirectory(sdk)
add_subdirectory(app)

if(ANDROID_PLATFORM)
	add_subdirectory(android/app)
else(ANDROID_PLATFORM)
	enable_testing()
	add_subdirectory(tests)
endif(ANDROID_PLATFORM)


