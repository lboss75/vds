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

void vds::const_data_buffer::remove(size_t start, size_t size) {
  vds_assert(this->size_ > start + size);
  this->size_ -= size;
  memmove(this->data_ + start, this->data_ + start + size, this->size_ - start);
}
