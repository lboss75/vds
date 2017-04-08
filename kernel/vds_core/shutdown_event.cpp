/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "shutdown_event.h"


vds::shutdown_event::shutdown_event()
#ifdef _WIN32
    : event_(TRUE)
#else
    : is_shuting_down_(false)
#endif
{

}

vds::shutdown_event::~shutdown_event()
{

}

bool vds::shutdown_event::is_shuting_down() const
{
#ifdef _WIN32
    return this->event_.wait(0);
#else
    return this->is_shuting_down_;
#endif
}

void vds::shutdown_event::set()
{
#ifdef _WIN32
    this->event_.set();
#else
    this->is_shuting_down_ = true;
#endif
    (*this)();
}
