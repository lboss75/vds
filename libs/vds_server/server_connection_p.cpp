#include "stdafx.h"
#include "server_connection_p.h"

vds::_server_connection::_server_connection(
  const service_provider & sp,
  server_connection * owner)
  : sp_(sp),
  owner_(owner)
{
}
