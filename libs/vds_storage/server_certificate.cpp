/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_certificate.h"

std::string vds::server_certificate::server_id(const vds::certificate& cert)
{
  return cert.subject();
}
