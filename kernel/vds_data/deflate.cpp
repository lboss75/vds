/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/deflate_p.h"

vds::deflate::deflate()
  : impl_(new _deflate_handler(Z_DEFAULT_COMPRESSION))
{
}

vds::deflate::deflate(int compression_level)
  : impl_(new _deflate_handler(compression_level))
{
}

vds::deflate::~deflate()
{
  delete this->impl_;
}

void vds::deflate::update(
  const void * input_data,
  size_t input_size,
  void * output_data,
  size_t output_size,
  size_t & readed,
  size_t & written)
{
  this->impl_->update_data(
      input_data,
      input_size,
      output_data,
      output_size,
      readed,
      written);
}
