if(ANDROID_PLATFORM)
set(OPENSSL_INCLUDE_DIR /home/vadim/projects/openssl/build/include)
set(OPENSSL_LIBRARIES
 /home/vadim/projects/openssl/build/lib/libssl.a
  /home/vadim/projects/openssl/build/lib/libcrypto.a)
else(ANDROID_PLATFORM)
    message(FATAL_ERROR "Invalid usage FindOpenSSL.cmake")
endif(ANDROID_PLATFORM)
