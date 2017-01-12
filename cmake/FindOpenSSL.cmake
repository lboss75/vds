find_path(OPENSSL_INCLUDE_DIR openssl/conf.h
  PATHS $ENV{OPENSSL_ROOT_DIR}/include
)

find_library(LIBSSL_LIBRARY
  NAMES libssl.lib
  PATHS $ENV{OPENSSL_ROOT_DIR}/lib
)

find_library(LIBCRYPTO_LIBRARY
  NAMES libcrypto.lib
  PATHS $ENV{OPENSSL_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OPENSSL
  DEFAULT_MSG
  OPENSSL_INCLUDE_DIR
  LIBCRYPTO_LIBRARY
)

set(OPENSSL_LIBRARIES
  ${LIBSSL_LIBRARY} ${LIBCRYPTO_LIBRARY}
)
