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

vds::http_message vds::http_response::create_message(const service_provider & sp) const
{
  std::list<std::string> headers(this->headers_);

  std::stringstream stream;
  stream << this->protocol_ << " " << this->code_ << " " << this->comment_;
  headers.push_front(stream.str());

  return http_message(sp, headers);
}

vds::http_message vds::http_response::simple_text_response(
  const service_provider & sp,
  const std::string & body,
  int result_code /*= HTTP_OK*/,
  const std::string & message /*= "OK"*/)
{
  http_response response(result_code, message);
  response.add_header("Content-Type", "text/html; charset=utf-8");
  response.add_header("Content-Length", std::to_string(body.length()));
  auto result = response.create_message(sp);
  if (!body.empty()) {
    auto buffer = std::make_shared<std::string>(body);
    result.body()->write_async((const uint8_t *)buffer->c_str(), buffer->length())
      .execute(
        [sp, result, buffer](const std::shared_ptr<std::exception> & ex) {
      if (!ex) {
        result.body()->write_async(nullptr, 0).execute(
          [sp](const std::shared_ptr<std::exception> & ex) {
          if (ex) {
            sp.unhandled_exception(ex);
          }
        });
      }
      else {
        sp.unhandled_exception(ex);
      }
    });
  }
  else {
    result.body()->write_async(nullptr, 0).execute(
      [sp](const std::shared_ptr<std::exception> & ex) {
      if (ex) {
        sp.unhandled_exception(ex);
      }
    });
  }
  return result;
}

vds::http_message vds::http_response::status_response(
    const service_provider & sp,
    int result_code,
    const std::string & message)
{
  http_response response(result_code, message);
  auto result = response.create_message(sp);

  result.body()->write_async(nullptr, 0).execute(
      [sp](const std::shared_ptr<std::exception> & ex) {
        if (ex) {
          sp.unhandled_exception(ex);
        }
      });

  return result;
}


