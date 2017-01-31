/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "crypto_service.h"


void vds::crypto_service::register_services(service_registrator &)
{
}

static int number_of_locks = 0;
static ssl_lock * ssl_locks = nullptr;

static void ssl_lock_callback(int mode, int n, const char *file, int line)
{
  if (mode & CRYPTO_LOCK) {
    EnterCriticalSection(&ssl_locks[n]);
  }
  else {
    LeaveCriticalSection(&ssl_locks[n]);
  }
}

static CRYPTO_dynlock_value* ssl_lock_dyn_create_callback(const char *file, int line)
{
  CRYPTO_dynlock_value *l = (CRYPTO_dynlock_value*)malloc(sizeof(CRYPTO_dynlock_value));
  InitializeCriticalSection(&l->lock);
  return l;
}

static void ssl_lock_dyn_callback(int mode, CRYPTO_dynlock_value* l, const char *file, int line)
{
  if (mode & CRYPTO_LOCK) {
    EnterCriticalSection(&l->lock);
  }
  else {
    LeaveCriticalSection(&l->lock);
  }
}

static void ssl_lock_dyn_destroy_callback(CRYPTO_dynlock_value* l, const char *file, int line)
{
  DeleteCriticalSection(&l->lock);
  free(l);
}


void vds::crypto_service::start(const service_provider &)
{
  auto rnd_seed = time(NULL);
  RAND_seed(&rnd_seed, sizeof(rnd_seed));

  number_of_locks = CRYPTO_num_locks();
  if (number_of_locks > 0) {
    ssl_locks = (ssl_lock*)malloc(number_of_locks * sizeof(ssl_lock));
    for (int i = 0; i < number_of_locks; ++i) {
      InitializeCriticalSection(&ssl_locks[i]);
    }
  }

#ifdef _DEBUG
  CRYPTO_malloc_debug_init();
  CRYPTO_dbg_set_options(V_CRYPTO_MDEBUG_ALL);
  CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
#endif

  CRYPTO_set_locking_callback(&ssl_lock_callback);
  CRYPTO_set_dynlock_create_callback(&ssl_lock_dyn_create_callback);
  CRYPTO_set_dynlock_lock_callback(&ssl_lock_dyn_callback);
  CRYPTO_set_dynlock_destroy_callback(&ssl_lock_dyn_destroy_callback);

  SSL_load_error_strings();

  /* Load the human readable error strings for libcrypto */
  ERR_load_crypto_strings();

  /* Load all digest and cipher algorithms */
  OpenSSL_add_all_algorithms();

  /* Load config file, and other important initialisation */
  OPENSSL_config(NULL);
  
}

void vds::crypto_service::stop(const service_provider &)
{
  CRYPTO_set_locking_callback(NULL);
  CRYPTO_set_dynlock_create_callback(NULL);
  CRYPTO_set_dynlock_lock_callback(NULL);
  CRYPTO_set_dynlock_destroy_callback(NULL);

  /* Removes all digests and ciphers */
  EVP_cleanup();

  /* if you omit the next, a small leak may be left when you make use of the BIO (low level API) for e.g. base64 transformations */
  CRYPTO_cleanup_all_ex_data();

  /* Remove error strings */
  ERR_free_strings();

  if (nullptr != ssl_locks) {
    for (int i = 0; i < number_of_locks; ++i) {
      DeleteCriticalSection(&ssl_locks[i]);
    }

    free(ssl_locks);
    ssl_locks = nullptr;
    number_of_locks = 0;
  }
}