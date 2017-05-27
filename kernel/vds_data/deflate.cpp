/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "deflate_p.h"

vds::deflate::deflate()
  : compression_level_(Z_DEFAULT_COMPRESSION)
{
}

vds::deflate::deflate(int compression_level)
  : compression_level_(compression_level)
{
}

vds::_deflate_handler * vds::deflate::create_handler() const
{
  return new _deflate_handler(this->compression_level_);
}

void vds::deflate::delete_handler(_deflate_handler * handler)
{
  delete handler;
}

void vds::deflate::update_data(
  _deflate_handler * handler,
  const void * input_data,
  size_t input_size,
  void * output_data,
  size_t output_size,
  size_t & readed,
  size_t & written)
{
  handler->update_data(
      input_data,
      input_size,
      output_data,
      output_size,
      readed,
      written);
}
