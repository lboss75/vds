cmake_minimum_required(VERSION 2.6.2)

project(vds)

include(CMakeLists.inc)

add_subdirectory(kernel)
add_subdirectory(libs)
add_subdirectory(app)
add_subdirectory(tests)



