/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_serializer.h"


vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<unsigned char>>>>
vds::http_async_serializer::start_message(const std::list<std::string>& headers) {
  vds_assert(!this->write_body_);
  std::stringstream stream;
  for (auto& header : headers) {
    stream << header << "\r\n";
  }
  stream << "\r\n";

  auto data = stream.str();
  CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(reinterpret_cast<const uint8_t *>(data.c_str()), data.length(
  )));

  this->write_body_ = true;

  co_return std::make_shared<out_stream>(this->shared_from_this());
}

vds::async_task<vds::expected<void>> vds::http_async_serializer::stop() {
  return this->target_->write_async(nullptr, 0);
}

vds::async_task<vds::expected<void>> vds::http_async_serializer::out_stream::write_async(
  const uint8_t* data, size_t len) {

  if (0 != len) {
    co_await this->target_->target_->write_async(data, len);
  }
  else {
    this->target_->write_body_ = false;
  }

  co_return expected<void>();
}
