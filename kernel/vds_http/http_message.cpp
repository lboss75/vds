/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_message.h"

bool vds::http_message::get_header(
    const std::list<std::string> & headers,
    const std::string& name,
    std::string& value)
{
  for (auto& p : headers) {
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

bool vds::http_message::have_header(const std::list<std::string>& headers, const std::string & name)
{
	for (auto& p : headers) {
		//Start with
		if (
			p.size() > name.size()
			&& p[name.size()] == ':'
			&& !p.compare(0, name.size(), name)) {
			return true;
		}
	}

	return false;
}

vds::async_task<vds::expected<void>> vds::http_message::ignore_empty_body() const {
  const_data_buffer result;
  GET_EXPECTED_VALUE_ASYNC(result, co_await this->body_->read_all());
  vds_assert(0 == result.size());
  co_return vds::expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_message::ignore_body() const {
  auto buffer = std::make_shared<buffer_t>();
  CHECK_EXPECTED_ASYNC(co_await ignore_body(this->body_, buffer));
  co_return vds::expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_message::ignore_body(  
  const std::shared_ptr<stream_input_async<uint8_t>> & body,
  const std::shared_ptr<buffer_t>& buffer) {

  for(;;){
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await body->read_async(buffer->data_, sizeof(buffer->data_)));
    if (0 == readed) {
      co_return expected<void>();
    }
  }
}
