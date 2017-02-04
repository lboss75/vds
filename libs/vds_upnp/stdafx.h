#ifndef __VDS_UPNP_STDAFX_H_
#define __VDS_UPNP_STDAFX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "vds_core.h"

#ifdef _WIN32

#else//_WIN32

#include <miniupnpc.h>
#include <upnpcommands.h>
#include <upnperrors.h>

#endif//_WIN32

#endif // __VDS_UPNP_STDAFX_H_
