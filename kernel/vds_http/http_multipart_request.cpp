/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_multipart_request.h"
#include "string_utils.h"
#include "http_client.h"

static const char endl[] = "\r\n";

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

  const auto part = "--" + this->boundary_ + endl
    + "Content-Disposition: form-data; name=\"" + url_encode::encode(name) + "\"" + endl + endl
    + url_encode::encode(value) + endl;

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
  auto header = "--" + this->boundary_ + endl
    + "Content-Disposition: form-data; name=\"" + url_encode::encode(name) + "\"; filename=\"" + url_encode::encode(filename) + "\"" + endl
    + "Content-Type: " + content_type + endl
    + "Content-Length: " + std::to_string(size) + endl;
  for(const auto & h : headers) {
    header += h;
    header += endl;
  }
  header += endl;

  this->total_size_ += header.length();
  this->total_size_ += size;

  this->inputs_.push(std::make_shared<buffer_stream_input_async>(const_data_buffer(header.c_str(), header.length())));
  
  auto f = std::make_shared<file_stream_input_async>();
  CHECK_EXPECTED(f->open(body_file));
  this->inputs_.push(f);

  this->total_size_ += sizeof(endl) - 1;
  this->inputs_.push(std::make_shared<buffer_stream_input_async>(const_data_buffer(endl, sizeof(endl) - 1)));

  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_multipart_request::send(
  const std::shared_ptr<http_async_serializer> & output_stream) {

  const auto tail = "--" + this->boundary_ + "--" + endl;

  this->total_size_ += tail.length();
  this->inputs_.push(std::make_shared<buffer_stream_input_async>(const_data_buffer(tail.c_str(), tail.length())));

  this->headers_.push_back("Content-Length: " + std::to_string(this->total_size_));

  GET_EXPECTED_ASYNC(stream, co_await output_stream->start_message(this->headers_));

  uint8_t buffer[1024];
  while (!this->inputs_.empty()) {
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await this->inputs_.front()->read_async(buffer, sizeof(buffer)));
    if (0 == readed) {
      this->inputs_.pop();
    }
    else {
      CHECK_EXPECTED_ASYNC(co_await stream->write_async(buffer, readed));
    }
  }

  CHECK_EXPECTED_ASYNC(co_await stream->write_async(nullptr, 0));

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_multipart_request::send(
  const std::shared_ptr<http_client>& client,
  lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> response_handler) {
  const auto tail = "--" + this->boundary_ + "--" + endl;

  this->total_size_ += tail.length();
  this->inputs_.push(std::make_shared<buffer_stream_input_async>(const_data_buffer(tail.c_str(), tail.length())));

  this->headers_.push_back("Content-Length: " + std::to_string(this->total_size_));

  return client->send_headers(
    this->headers_,
    std::move(response_handler),
    [inputs = std::move(this->inputs_)](std::shared_ptr<stream_output_async<uint8_t>> stream) mutable ->async_task<expected<void>> {
      uint8_t buffer[1024];
      while (!inputs.empty()) {
        size_t readed;
        GET_EXPECTED_VALUE_ASYNC(readed, co_await inputs.front()->read_async(buffer, sizeof(buffer)));
        if (0 == readed) {
          inputs.pop();
        }
        else {
          CHECK_EXPECTED_ASYNC(co_await stream->write_async(buffer, readed));
        }
      }

      CHECK_EXPECTED_ASYNC(co_await stream->write_async(nullptr, 0));
  });
}

vds::async_task<vds::expected<vds::const_data_buffer>> vds::http_multipart_request::send(
  const std::shared_ptr<http_client>& client,
  lambda_holder_t<async_task<expected<void>>, http_message> response_handler) {
  const auto tail = "--" + this->boundary_ + "--" + endl;

  this->total_size_ += tail.length();
  this->inputs_.push(std::make_shared<buffer_stream_input_async>(const_data_buffer(tail.c_str(), tail.length())));

  this->headers_.push_back("Content-Length: " + std::to_string(this->total_size_));

  return client->send_headers(
    this->headers_,
    std::move(response_handler),
    [inputs = std::move(this->inputs_)](std::shared_ptr<stream_output_async<uint8_t>> stream) mutable->async_task<expected<void>> {
    uint8_t buffer[1024];
    while (!inputs.empty()) {
      size_t readed;
      GET_EXPECTED_VALUE_ASYNC(readed, co_await inputs.front()->read_async(buffer, sizeof(buffer)));
      if (0 == readed) {
        inputs.pop();
      }
      else {
        CHECK_EXPECTED_ASYNC(co_await stream->write_async(buffer, readed));
      }
    }

    CHECK_EXPECTED_ASYNC(co_await stream->write_async(nullptr, 0));
    co_return expected<void>();
  });
}


void vds::http_multipart_request::add_header(const std::string& header) {
  this->headers_.push_back(header);
}

