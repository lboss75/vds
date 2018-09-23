/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_response.h"

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
  headers.push_front("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  if (!content_type.empty()) {
    headers.push_back("Content-Type:" + content_type);
  }
  headers.push_back("Content-Length:" + std::to_string(body.length()));

  return http_message(headers, std::make_shared<buffer_input_stream_async>(const_data_buffer(body.c_str(), body.length())));
}

vds::http_message vds::http_response::redirect(const std::string& location) {
  std::list<std::string> headers;
  headers.push_front("HTTP/1.0 302 Found");
  headers.push_back("Location:" + location);

  return http_message(headers, std::make_shared<buffer_input_stream_async>(const_data_buffer()));
}

vds::http_message vds::http_response::status_response(
    int result_code,
    const std::string & message)
{
  std::list<std::string> headers;
  headers.push_front("HTTP/1.0 " + std::to_string(result_code) + " " + message);

  return http_message(headers, std::make_shared<buffer_input_stream_async>(const_data_buffer()));
}

vds::http_message vds::http_response::file_response(
  const filename & body_file,
  const std::string & out_filename,
  const std::string & content_type,
  int result_code /*= HTTP_OK*/,
    const std::string & message /*= "OK"*/){

  std::list<std::string> headers;
  headers.push_front("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  headers.push_front("Content-Type:" + content_type);
  headers.push_front("Content-Length:" + std::to_string(file::length(body_file)));
  headers.push_front("Content-Disposition:attachment; filename=\"" + out_filename + "\"");
  
  return http_message(headers, std::make_shared<file_input_stream_async>(body_file));
}

vds::http_message vds::http_response::file_response(
  const const_data_buffer& body,
  const std::string& filename,
  const std::string& content_type,
  int result_code,
  const std::string& message) {

  std::list<std::string> headers;
  headers.push_front("HTTP/1.0 " + std::to_string(result_code) + " " + message);
  headers.push_front("Content-Type:" + content_type);
  headers.push_front("Content-Length:" + std::to_string(body.size()));
  headers.push_front("Content-Disposition:attachment; filename=\"" + filename + "\"");
  return http_message(headers, std::make_shared<buffer_input_stream_async>(body));

}

