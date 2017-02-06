
if(WIN32)
  include (CheckIncludeFileCXX)
  CHECK_INCLUDE_FILE_CXX(natupnp.h HAVE_NATIVE_NATUPNP)
  SET(UPNP_LIBRARIES)
else()
  find_path(
    UPNP_INCLUDE_DIR
    NAMES miniupnpc.h
    PATH_SUFFIXES miniupnpc)
    
  set(UPNP_NAMES ${UPNP_NAMES} miniupnpc libminiupnpc)
  find_library(UPNP_LIBRARY NAMES ${UPNP_NAMES})
  SET(UPNP_LIBRARIES ${UPNP_LIBRARY})
endif()
