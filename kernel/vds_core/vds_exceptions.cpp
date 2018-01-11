/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vds_exceptions.h"

vds::vds_exceptions::not_found::not_found()
    : std::runtime_error("Not found"){
}

vds::vds_exceptions::invalid_operation::invalid_operation()
    : std::runtime_error("Invalid operation"){
}

vds::vds_exceptions::signature_validate_error::signature_validate_error()
	: std::runtime_error("Invalid data validation") {
}
