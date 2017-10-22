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

void  vds::inflate::write(
  const service_provider & sp,
  const uint8_t * input_data,
  size_t input_size)
{
  this->impl_->write(sp, input_data, input_size);
}
//////////////////////////////////////////////////////
vds::inflate_async::inflate_async(stream_async<uint8_t> & target)
: impl_(new _inflate_async_handler(target))
{
}

vds::inflate_async::~inflate_async()
{
  delete this->impl_;
}

vds::async_task<>  vds::inflate_async::write_async(
  const service_provider & sp, 
  const uint8_t * input_data,
  size_t input_size)
{
  return this->impl_->write_async(sp, input_data, input_size);
}
