/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "socket_task.h"

vds::socket_task::socket_task()
{
#ifdef _WIN32
  ZeroMemory(&this->overlapped_, sizeof(this->overlapped_)); 
#endif//_WIN32
}

#ifdef _WIN32
vds::socket_task::~socket_task()
{

}
#endif//_WIN32