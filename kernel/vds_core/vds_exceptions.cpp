/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vds_exceptions.h"

vds::vds_exceptions::not_found::not_found()
: std::runtime_error("Not found"){
}
