/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "socket_task_p.h"

vds::_socket_task::_socket_task()
{
#ifdef _WIN32
  ZeroMemory(&this->overlapped_, sizeof(this->overlapped_)); 
#endif//_WIN32
}

vds::_socket_task::~_socket_task()
{
}
