/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "inflate.h"
#include "private/inflate_p.h"

vds::inflate::inflate(stream<uint8_t> & target)
: impl_(new _inflate_handler(target))
{
}

vds::inflate::~inflate()
{
  delete this->impl_;
}

void vds::inflate::write(
  const uint8_t * input_data,
  size_t input_size)
{
  this->impl_->write(input_data, input_size);
}
