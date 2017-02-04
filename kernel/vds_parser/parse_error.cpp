/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "parse_error.h"

vds::parse_error::parse_error(const std::string & stream_name, int line, int column, const std::string& message)
: std::runtime_error(string_format("%s(%d, %d): %s", stream_name.c_str(), line, column, message.c_str()))
{
}

