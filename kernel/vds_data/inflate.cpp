/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "inflate.h"
#include "inflate_p.h"

vds::inflate::inflate()
{
}

vds::_inflate_handler * vds::inflate::create_handler() const
{
  return new _inflate_handler();
}

void vds::inflate::delete_handler(_inflate_handler * handler)
{
  delete handler;
}

void vds::inflate::update_data(
  _inflate_handler * handler,
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

