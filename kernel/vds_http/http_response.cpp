/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_response.h"

vds::http_response::http_response(
  const http_message & message)
{
  auto p = message.headers().begin();
  auto items = split_string(*p++, ' ', true, true);
  
  this->protocol_ = items.front();
  items.pop_front();

  this->code_ = atoi(items.front().c_str());
  items.pop_front();

  this->comment_ = items.front();
  items.pop_front();
  while(!items.empty()) {
    this->comment_ += ' ';
    this->comment_ += items.front();
    items.pop_front();
  }

  while (message.headers().end() != p) {
    this->headers_.push_back(*p++);
  }
}

vds::http_response::http_response(
  int code,
  const std::string & comment)
: protocol_("HTTP/1.0"), code_(code), comment_(comment)
{
}


vds::async_task<vds::expected<void>> vds::http_response::simple_text_response(
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::string & body,
  const std::string & content_type /*= "text/html; charset=utf-8"*/,
  int result_code /*= HTTP_OK*/,
  const std::string & message /*= "OK"*/)
{
  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  if (!content_type.empty()) {
    headers.push_back("Content-Type:" + content_type);
  }
  headers.push_back("Content-Length:" + std::to_string(body.length()));
  
  GET_EXPECTED_ASYNC(stream, co_await output_stream->start_message(headers));
  CHECK_EXPECTED_ASYNC(co_await stream->write_async((const uint8_t *)body.c_str(), body.length()));

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_response::simple_text_response(
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::shared_ptr<stream_input_async<uint8_t>> & body,
  uint64_t body_size,
  const std::string & content_type,
  int result_code,
  const std::string& message) {

  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  if (!content_type.empty()) {
    headers.push_back("Content-Type:" + content_type);
  }
  headers.push_back("Content-Length:" + std::to_string(body_size));

  GET_EXPECTED_ASYNC(stream, co_await output_stream->start_message(headers));
  CHECK_EXPECTED_ASYNC(co_await body->copy_to(stream));

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_response::redirect(
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::string& location) {
  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 302 Found");
  headers.push_back("Location:" + location);

  GET_EXPECTED_ASYNC(stream, co_await output_stream->start_message(headers));
  CHECK_EXPECTED_ASYNC(co_await stream->write_async(nullptr, 0));

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_response::status_response(
  const std::shared_ptr<http_async_serializer> & output_stream,
  int result_code,
    const std::string & message)
{
  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 " + std::to_string(result_code) + " " + message);

  GET_EXPECTED_ASYNC(stream, co_await output_stream->start_message(headers));
  CHECK_EXPECTED_ASYNC(co_await stream->write_async(nullptr, 0));

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_response::file_response(
  const std::shared_ptr<http_async_serializer> & output_stream,
  const filename & body_file,
  const std::string & out_filename,
  const std::string & content_type,
  int result_code /*= HTTP_OK*/,
    const std::string & message /*= "OK"*/){

  GET_EXPECTED_ASYNC(body_size, file::length(body_file));

  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  headers.push_back("Content-Type:" + content_type);
  headers.push_back("Content-Length:" + std::to_string(body_size));
  headers.push_back("Content-Disposition:attachment; filename=\"" + out_filename + "\"");

  auto f = std::make_shared<file_stream_input_async>();
  CHECK_EXPECTED_ASYNC(f->open(body_file));

  GET_EXPECTED_ASYNC(stream, co_await output_stream->start_message(headers));
  CHECK_EXPECTED_ASYNC(co_await f->copy_to(stream));

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_response::file_response(
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::shared_ptr<stream_input_async<uint8_t>>& body,
  uint64_t body_size,
  const std::string & filename,
  const const_data_buffer & file_hash,
  const std::string & content_type,
  int result_code,
  const std::string & message)
{
  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  headers.push_back("Content-Type:" + content_type);
  headers.push_back("Content-Length:" + std::to_string(body_size));
  headers.push_back("Content-Disposition:attachment; filename=\"" + filename + "\"");
  if(0 < file_hash.size()) {
    headers.push_back("X-VDS-SHA256:" + base64::from_bytes(file_hash));
  }

  GET_EXPECTED_ASYNC(stream, co_await output_stream->start_message(headers));
  CHECK_EXPECTED_ASYNC(co_await body->copy_to(stream));

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_response::file_response(
  const std::shared_ptr<http_async_serializer> & output_stream,
  const const_data_buffer& body,
  const std::string& filename,
  const const_data_buffer & file_hash,
  const std::string& content_type,
  int result_code,
  const std::string& message) {

  return file_response(
    output_stream,
    std::make_shared<buffer_stream_input_async>(body),
    body.size(),
    filename,
    file_hash,
    content_type,
    result_code,
    message);
}

