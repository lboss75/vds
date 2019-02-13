/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_multipart_request.h"
#include "string_utils.h"

vds::http_multipart_request::http_multipart_request(
  const std::string& method,
  const std::string& url,
  const std::string& agent)
: total_size_(0) {
  this->headers_.push_back(method + " " + url + " " + agent);

  uint8_t rnd_bytes[10];
  for(size_t i = 0; i < sizeof(rnd_bytes)/ sizeof(rnd_bytes[0]); ++i) {
    rnd_bytes[i] = (uint8_t)std::rand();
  }

  this->boundary_ = base64::from_bytes(rnd_bytes, sizeof(rnd_bytes));
  replace_string(this->boundary_, "=", "");
  replace_string(this->boundary_, "+", "");
  replace_string(this->boundary_, "/", "");

  this->headers_.push_back("Content-Type: multipart/form-data; boundary=" + this->boundary_);
}

void vds::http_multipart_request::add_string(const std::string& name, const std::string& value) {

  const auto part = "--" + this->boundary_ + "\r\n"
    + "Content-Disposition: form-data; name=\"" + url_encode::encode(name) + "\"\r\n\r\n"
    + url_encode::encode(value) + "\r\n";

  this->total_size_ += part.length();
  this->inputs_.push(std::make_shared<buffer_stream_input_async>(const_data_buffer(part.c_str(), part.length())));
}

vds::expected<void> vds::http_multipart_request::add_file(
  const std::string & name,
  const filename& body_file,
  const std::string& filename,
  const std::string& content_type,
  const std::list<std::string> & headers) {

  GET_EXPECTED(size, file::length(body_file));
  auto header = "--" + this->boundary_ + "\r\n"
    + "Content-Disposition: form-data; name=\"" + url_encode::encode(name) + "\"; filename=\"" + url_encode::encode(filename) + "\"\r\n"
    + "Content-Type: " + content_type + "\r\n"
    + "Content-Length: " + std::to_string(size) + "\r\n";
  for(const auto & h : headers) {
    header += h;
    header += "\r\n";
  }
  header += "\r\n";

  this->total_size_ += header.length();
  this->total_size_ += size;

  this->inputs_.push(std::make_shared<buffer_stream_input_async>(const_data_buffer(header.c_str(), header.length())));
  
  auto f = std::make_shared<file_stream_input_async>();
  CHECK_EXPECTED(f->open(body_file));
  this->inputs_.push(f);

  return expected<void>();
}

vds::http_message vds::http_multipart_request::get_message() {

  const auto tail = "--" + this->boundary_ + "--\r\n";

  this->total_size_ += tail.length();
  this->inputs_.push(std::make_shared<buffer_stream_input_async>(const_data_buffer(tail.c_str(), tail.length())));

  this->headers_.push_back("Content-Length: " + std::to_string(this->total_size_));

  return http_message(this->headers_, std::make_shared<multipart_body>(std::move(this->inputs_)));
}

void vds::http_multipart_request::add_header(const std::string& header) {
  this->headers_.push_back(header);
}

vds::http_multipart_request::multipart_body::multipart_body(
  std::queue<std::shared_ptr<stream_input_async<uint8_t>>>&& inputs)
: inputs_(std::move(inputs)) {
}

vds::async_task<vds::expected<size_t>> vds::http_multipart_request::multipart_body::read_async(uint8_t* buffer, size_t len) {
  while(!this->inputs_.empty()) {
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await this->inputs_.front()->read_async(buffer, len));
    if(0 != readed) {
      co_return readed;
    }

    this->inputs_.pop();
  }

  co_return 0;
}



