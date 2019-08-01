/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/deflate_p.h"
#include "const_data_buffer.h"

vds::deflate::deflate()
  : impl_(nullptr)
{
}

vds::deflate::~deflate() {
  delete this->impl_;
}

vds::expected<std::shared_ptr<vds::deflate>> vds::deflate::create(const std::shared_ptr<stream_output_async<uint8_t>> & target) {
  auto impl = std::make_unique<_deflate_handler>();
  CHECK_EXPECTED(impl->create(target, Z_DEFAULT_COMPRESSION));
  return std::make_shared<deflate>(impl.release());
}

vds::expected<std::shared_ptr<vds::deflate>> vds::deflate::create(const std::shared_ptr<stream_output_async<uint8_t>> & target, int compression_level) {
  auto impl = std::make_unique<_deflate_handler>();
  CHECK_EXPECTED(impl->create(target, compression_level));
  return std::make_shared<deflate>(impl.release());
}

vds::expected<vds::const_data_buffer> vds::deflate::compress(  
  const uint8_t * data,
  size_t len)
{
  auto result = std::make_shared<collect_data>();
  _deflate_handler df;
  CHECK_EXPECTED(df.create(result, Z_DEFAULT_COMPRESSION));

  thread_unprotect unprotect;
  CHECK_EXPECTED(df.write_async(data, len).get());
  CHECK_EXPECTED(df.write_async(nullptr, 0).get());
  
  return result->move_data();  
}

vds::expected<vds::const_data_buffer> vds::deflate::compress(  
  const const_data_buffer & data)
{
  return compress(data.data(), data.size());
}

vds::async_task<vds::expected<void>> vds::deflate::write_async( const uint8_t *data, size_t len) {
  return this->impl_->write_async(data, len);
}
