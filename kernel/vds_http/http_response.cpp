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
  auto buffer = std::make_shared<std::string>(body);
  result->body()->write_async(sp, (const uint8_t *)buffer->c_str(), buffer->length())
    .execute(
      [sp, result, buffer](const std::shared_ptr<std::exception> & ex) {
        if(!ex){
        result->body()->write_async(sp, nullptr, 0).execute(
          [sp](const std::shared_ptr<std::exception> & ex) {
            sp.unhandled_exception(ex);
          });
        } else {
          sp.unhandled_exception(ex);
        }
      });
  return result;
}



