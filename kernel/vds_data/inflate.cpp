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

vds::inflate::inflate(_inflate_handler* impl)
: impl_(impl) {
}

vds::inflate::inflate(inflate&& origin) noexcept {
  this->impl_ = origin.impl_;
  origin.impl_ = nullptr;
}

vds::inflate::~inflate() {
  delete this->impl_;
}

vds::expected<std::shared_ptr<vds::inflate>> vds::inflate::create(const std::shared_ptr<stream_output_async<uint8_t>> & target) {
  auto impl = std::make_unique<_inflate_handler>();
  CHECK_EXPECTED(impl->create(target));
  return std::make_shared<inflate>(impl.release());
}

vds::expected<vds::const_data_buffer> vds::inflate::decompress(  
  const void * data,
  size_t size)
{
	auto result = std::make_shared<collect_data>();
  _inflate_handler inf;
  CHECK_EXPECTED(inf.create(result));

  CHECK_EXPECTED(inf.write_async((const uint8_t *)data, size).get());
  CHECK_EXPECTED(inf.write_async(nullptr, 0).get());

	return result->move_data();
}

vds::async_task<vds::expected<void>> vds::inflate::write_async( const uint8_t *data, size_t len) {
  return this->impl_->write_async(data, len);
}

vds::inflate& vds::inflate::operator=(inflate&& origin) noexcept {
  delete this->impl_;
  this->impl_ = origin.impl_;
  origin.impl_ = nullptr;
  return *this;
}

