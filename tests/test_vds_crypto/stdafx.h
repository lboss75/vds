#ifndef __VDS_TEST_CRYPTO_STDAFX_H_
#define __VDS_TEST_CRYPTO_STDAFX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "gtest/gtest.h"

#include "vds_core.h"
#include "vds_crypto.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "crypt32")
#endif

#endif // __VDS_TEST_CRYPTO_STDAFX_H_
