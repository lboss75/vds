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

void vds::http_message::ignore_empty_body() const {
  this->body_->read_all().execute([](const std::shared_ptr<std::exception> & ex, const const_data_buffer & result) {
    vds_assert(0 == result.size());
  });
}

void vds::http_message::ignore_body() const {
  auto buffer = std::make_shared<buffer_t>();
  return ignore_body(this->body_, buffer)
  .execute([buffer](const std::shared_ptr<std::exception> & ex) {
    vds_assert(!ex);
  });
}

vds::async_task<> vds::http_message::ignore_body(
  const std::shared_ptr<continuous_buffer<uint8_t>> & body,
  const std::shared_ptr<buffer_t>& buffer) {
  return body->read_async(buffer->data_, sizeof(buffer->data_))
    .then([body, buffer](size_t readed) -> async_task<> {
    if (0 == readed) {
      return async_task<>::empty();
    }
    else {
      return ignore_body(body, buffer);
    }
  });
}
