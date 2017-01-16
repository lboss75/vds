/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "crypto_exception.h"

vds::crypto_exception::crypto_exception(const std::string & message, int error_code)
  : std::runtime_error(message)
{
}
