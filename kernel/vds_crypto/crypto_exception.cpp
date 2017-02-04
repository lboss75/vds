/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "crypto_exception.h"
#include "string_format.h"

vds::crypto_exception::crypto_exception(const std::string & message, int error_code)
  : std::runtime_error(string_format("%s: %s", message.c_str(), ERR_error_string(error_code, nullptr)))
{
}
