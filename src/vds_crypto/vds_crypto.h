#ifndef __VDS_CRYPTO_VDS_CRYPTO_H_
#define __VDS_CRYPTO_VDS_CRYPTO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "crypto_service.h"
#include "symmetriccrypto.h"
#include "asymmetriccrypto.h"
#include "ssl_peer.h"

#ifdef _WIN32
#pragma comment(lib, "crypt32.lib")
#endif


#endif // __VDS_CRYPTO_VDS_CRYPTO_H_
