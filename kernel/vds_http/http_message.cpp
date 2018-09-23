/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_message.h"

bool vds::http_message::get_header(const std::string& name, std::string& value) const
{
  for (auto& p : this->headers_) {
    //Start with
    if (
      p.size() > name.size()
      && p[name.size()] == ':'
      && !p.compare(0, name.size(), name)) {
      value = p.substr(name.size() + 1);
      trim(value);
      return true;
    }
  }

  return false;
}

vds::async_task<void> vds::http_message::ignore_empty_body(const service_provider &sp) const {
  auto result = co_await this->body_->read_all(sp);
  vds_assert(0 == result.size());
}

vds::async_task<void> vds::http_message::ignore_body(const service_provider &sp) const {
  auto buffer = std::make_shared<buffer_t>();
  co_await ignore_body(sp, this->body_, buffer);
}

vds::async_task<void> vds::http_message::ignore_body(
  const service_provider &sp,
  const std::shared_ptr<input_stream_async<uint8_t>> & body,
  const std::shared_ptr<buffer_t>& buffer) {

  for(;;){
    auto readed = co_await body->read_async(sp, buffer->data_, sizeof(buffer->data_));
    if (0 == readed) {
      co_return;
    }
  }
}
