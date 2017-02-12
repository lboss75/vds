/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_logic.h"

vds::server_logic::server_logic(const vds::service_provider& sp)
: http_router(sp)
{

}
