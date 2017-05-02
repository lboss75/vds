/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "cert_manager.h"
#include "cert_manager_p.h"

bool vds::cert_manager::validate(
  const certificate& cert)
{
  return true;
}

////////////////////////////////////
bool vds::_cert_manager::validate(
  const certificate& cert)
{
  return true;
}
