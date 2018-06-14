cmake_minimum_required(VERSION 3.4.1)

project(vds)

include(CMakeLists.inc)

add_subdirectory(kernel)
add_subdirectory(libs)
add_subdirectory(app)
add_subdirectory(tests)

if(DANDROID_PLATFORM)
add_subdirectory(android/app)
endif(DANDROID_PLATFORM)


