/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/socket_task_p.h"

vds::_socket_task::_socket_task()
{
#ifdef _WIN32
  ZeroMemory(&this->overlapped_, sizeof(this->overlapped_)); 
#endif//_WIN32
  this->leak_detect_.name_ = "_socket_task";
  this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector) {
	  this->dump(collector);
  };
}

vds::_socket_task::~_socket_task()
{
}

void vds::_socket_task::dump(leak_detect_collector * collector) {

}
