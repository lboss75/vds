/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "inflate.h"
#include "private/inflate_p.h"

vds::inflate::inflate()
: impl_(new _inflate_handler())
{
}

vds::inflate::~inflate()
{
  delete this->impl_;
}

void vds::inflate::update_data(
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
