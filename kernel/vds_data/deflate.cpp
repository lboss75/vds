/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/deflate_p.h"
#include "const_data_buffer.h"

vds::deflate::deflate(const std::shared_ptr<stream_output_async<uint8_t>> & target)
  : impl_(new _deflate_handler(target, Z_DEFAULT_COMPRESSION))
{
}

vds::deflate::deflate(const std::shared_ptr<stream_output_async<uint8_t>> & target, int compression_level)
  : impl_(new _deflate_handler(target, compression_level))
{
}

vds::deflate::~deflate() {
  delete this->impl_;
}

vds::const_data_buffer vds::deflate::compress(
  const service_provider * sp,
  const uint8_t * data,
  size_t len)
{
  auto result = std::make_shared<collect_data<uint8_t>>();
  _deflate_handler df(result, Z_DEFAULT_COMPRESSION);
  
  df.write_async(sp, data, len).get();
  df.write_async(sp, nullptr, 0).get();
  
  return result->move_data();  
}

vds::const_data_buffer vds::deflate::compress(
  const service_provider * sp,
  const const_data_buffer & data)
{
  return compress(sp, data.data(), data.size());
}

std::future<void> vds::deflate::write_async(const vds::service_provider *sp, const uint8_t *data, size_t len) {
  return this->impl_->write_async(sp, data, len);
}
