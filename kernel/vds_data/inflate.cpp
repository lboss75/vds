/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "inflate.h"
#include "private/inflate_p.h"

vds::inflate::inflate()
: impl_(nullptr)
{
}

vds::inflate::~inflate() {
  delete this->impl_;
}

vds::expected<void> vds::inflate::create(const std::shared_ptr<stream_output_async<uint8_t>> & target) {
  vds_assert(nullptr == this->impl_);
  this->impl_ = new _inflate_handler();

  return this->impl_->create(target);
}

vds::expected<vds::const_data_buffer> vds::inflate::decompress(  
  const void * data,
  size_t size)
{
	auto result = std::make_shared<collect_data<uint8_t>>();
  _inflate_handler inf;
  CHECK_EXPECTED(inf.create(result));

  CHECK_EXPECTED(inf.write_async((const uint8_t *)data, size).get());
  CHECK_EXPECTED(inf.write_async(nullptr, 0).get());

	return result->move_data();
}

vds::async_task<vds::expected<void>> vds::inflate::write_async( const uint8_t *data, size_t len) {
  return this->impl_->write_async(data, len);
}

