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

std::shared_ptr<vds::http_message> vds::http_response::create_message() const
{
  std::list<std::string> headers(this->headers_);

  std::stringstream stream;
  stream << this->protocol_ << " " << this->code_ << " " << this->comment_;
  headers.push_front(stream.str());

  return std::make_shared<http_message>(headers);
}

std::shared_ptr<vds::http_message> vds::http_response::simple_text_response(
  const service_provider & sp,
  const std::string & body)
{
  http_response response(http_response::HTTP_OK, "OK");
  response.add_header("Content-Type", "text/html; charset=utf-8");
  response.add_header("Content-Length", std::to_string(body.length()));
  auto result = response.create_message();
  result->body()->write_all_async(sp, (const uint8_t *)body.c_str(), body.length())
    .wait(
      [result](const service_provider & sp) {
    result->body()->write_all_async(sp, nullptr, 0).wait(
      [](const service_provider & sp) {},
      [](const service_provider & sp, std::exception_ptr ex) {},
      sp);
  },
      [](const service_provider & sp, std::exception_ptr ex) {},
    sp);
  return result;
}



