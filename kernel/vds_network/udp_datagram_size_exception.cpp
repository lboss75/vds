/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "udp_datagram_size_exception.h"

vds::udp_datagram_size_exception::udp_datagram_size_exception()
: std::runtime_error("UDP Message too long") {
}
