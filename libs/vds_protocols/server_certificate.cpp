/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_certificate.h"

vds::guid vds::server_certificate::server_id(const vds::certificate& cert)
{
  auto subject = cert.subject();
  return guid::parse(subject.substr(31));
}
