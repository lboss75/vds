/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "shutdown_event.h"


vds::shutdown_event::shutdown_event()
: is_shuting_down_(false)
{

}

vds::shutdown_event::~shutdown_event()
{

}

vds::expected<void> vds::shutdown_event::create() {
#ifdef _WIN32
  CHECK_EXPECTED(this->event_.create(TRUE));
#endif
  return expected<void>();
}

vds::expected<void> vds::shutdown_event::set()
{
#ifdef _WIN32
  CHECK_EXPECTED(this->event_.set());
#endif
    this->is_shuting_down_ = true;
    return expected<void>();
}
