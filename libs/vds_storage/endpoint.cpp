/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "endpoint.h"
#include "storage_log.h"
#include "storage_log_p.h"

vds::storage_cursor<vds::endpoint>::storage_cursor(const istorage & storage)
  : _simple_storage_cursor<endpoint>(storage.get_storage_log()->get_endpoints())
{
}
