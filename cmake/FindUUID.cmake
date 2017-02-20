
find_path(UUID_INCLUDE_DIR uuid/uuid.h)
find_library(UUID_LIBRARY NAMES uuid PATHS /usr/lib/x86_64-linux-gnu)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    UUID
    REQUIRED_VARS UUID_LIBRARY UUID_INCLUDE_DIR
)

mark_as_advanced(
    UUID_FOUND
    UUID_LIBRARY UUID_INCLUDE_DIR
)

set(UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})
set(UUID_LIBRARIES ${UUID_LIBRARY})
  
if(NOT UUID_LIBRARIES)
  message(FATAL_ERROR "Could not find UUID")
else(NOT UUID_LIBRARIES)
  message(STATUS "Found UUID: ${UUID_LIBRARIES}")
endif(NOT UUID_LIBRARIES)

