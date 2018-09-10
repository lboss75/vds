/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "app.h"

vds::app * vds::app::the_app_ = nullptr;
#ifndef _WIN32
vds::barrier vds::app::stop_barrier;
#endif
