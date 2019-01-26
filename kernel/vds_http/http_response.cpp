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


vds::http_message vds::http_response::simple_text_response(
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

  return http_message(headers, std::make_shared<buffer_stream_input_async>(const_data_buffer(body.c_str(), body.length())));
}

vds::http_message vds::http_response::redirect(const std::string& location) {
  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 302 Found");
  headers.push_back("Location:" + location);

  return http_message(headers, std::make_shared<buffer_stream_input_async>(const_data_buffer()));
}

vds::http_message vds::http_response::status_response(
    int result_code,
    const std::string & message)
{
  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 " + std::to_string(result_code) + " " + message);

  return http_message(headers, std::make_shared<buffer_stream_input_async>(const_data_buffer()));
}

vds::expected<vds::http_message> vds::http_response::file_response(
  const filename & body_file,
  const std::string & out_filename,
  const std::string & content_type,
  int result_code /*= HTTP_OK*/,
    const std::string & message /*= "OK"*/){

  GET_EXPECTED(body_size, file::length(body_file));

  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  headers.push_back("Content-Type:" + content_type);
  headers.push_back("Content-Length:" + std::to_string(body_size));
  headers.push_back("Content-Disposition:attachment; filename=\"" + out_filename + "\"");

  auto f = std::make_shared<file_stream_input_async>();
  CHECK_EXPECTED(f->open(body_file));

  return http_message(headers, f);
}

vds::http_message vds::http_response::file_response(
  const std::shared_ptr<stream_input_async<uint8_t>>& body,
  uint64_t body_size,
  const std::string & filename,
  const std::string & content_type,
  int result_code,
  const std::string & message)
{
  std::list<std::string> headers;
  headers.push_back("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  headers.push_back("Content-Type:" + content_type);
  headers.push_back("Content-Length:" + std::to_string(body_size));
  headers.push_back("Content-Disposition:attachment; filename=\"" + filename + "\"");

  return http_message(headers, body);
}

vds::http_message vds::http_response::file_response(
  const const_data_buffer& body,
  const std::string& filename,
  const std::string& content_type,
  int result_code,
  const std::string& message) {

  return file_response(
    std::make_shared<buffer_stream_input_async>(body),
    body.size(),
    filename,
    content_type,
    result_code,
    message);
}

