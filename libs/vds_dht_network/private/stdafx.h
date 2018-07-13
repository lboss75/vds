#ifndef __VDS_DHT_NETWORK_STDAFX_H_
#define __VDS_DHT_NETWORK_STDAFX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "vds_core.h"
#include "vds_crypto.h"
#include "vds_network.h"

#ifdef _WIN32

#include <Ws2tcpip.h>

#endif

#define ThisModule "dht"
#define SyncModule "dht_sync"

#endif // __VDS_DHT_NETWORK_STDAFX_H_

