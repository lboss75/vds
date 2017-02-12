/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "crypto_service.h"


void vds::crypto_service::register_services(service_registrator &)
{
}

void vds::crypto_service::start(const service_provider &)
{
  auto rnd_seed = time(NULL);
  RAND_seed(&rnd_seed, sizeof(rnd_seed));


  SSL_library_init();
  SSL_load_error_strings();

  /* Load the human readable error strings for libcrypto */
  ERR_load_crypto_strings();

  ERR_load_BIO_strings();
  
  /* Load all digest and cipher algorithms */
  OpenSSL_add_all_algorithms();

  /* Load config file, and other important initialisation */
  OPENSSL_config(NULL);
  
  
}

void vds::crypto_service::stop(const service_provider &)
{
  /* Removes all digests and ciphers */
  EVP_cleanup();

  /* if you omit the next, a small leak may be left when you make use of the BIO (low level API) for e.g. base64 transformations */
  CRYPTO_cleanup_all_ex_data();

  /* Remove error strings */
  ERR_free_strings();
}