/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "shutdown_exception.h"

vds::shutdown_exception::shutdown_exception()
  : std::exception("Shutdown")
{
}
