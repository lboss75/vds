/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

vds::const_data_buffer::const_data_buffer(resizable_data_buffer&& other)
  : data_(other.data_), size_(other.size_), allocated_size_(other.allocated_size_)
{
  other.data_ = nullptr;
  other.size_ = 0;
  other.allocated_size_ = 0;
}
