/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "shutdown_event.h"


vds::shutdown_event::shutdown_event()
: is_shuting_down_(false)
#ifdef _WIN32
  , event_(TRUE)
#endif
{

}

vds::shutdown_event::~shutdown_event()
{

}

void vds::shutdown_event::set()
{
#ifdef _WIN32
    this->event_.set();
#endif
    this->is_shuting_down_ = true;
}
