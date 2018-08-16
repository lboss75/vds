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
  const std::string & content_type /*= "text/html; charset=utf-8"*/,
  int result_code /*= HTTP_OK*/,
  const std::string & message /*= "OK"*/)
{
  http_response response(result_code, message);
  if (!content_type.empty()) {
    response.add_header("Content-Type", content_type);
  }
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

vds::http_message vds::http_response::redirect(const service_provider& sp, const std::string& location) {
  http_response response(302, "Found");
  response.add_header("Location", location);
  auto result = response.create_message(sp);
  result.body()->write_async(nullptr, 0).execute(
    [sp](const std::shared_ptr<std::exception> & ex) {
    if (ex) {
      sp.unhandled_exception(ex);
    }
  });

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

vds::http_message vds::http_response::file_response(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<uint8_t>> & body,
    const std::string & content_type,
    const std::string & filename,
    size_t body_size,
    int result_code /*= HTTP_OK*/,
    const std::string & message /*= "OK"*/){

  http_response response(result_code, message);
  response.add_header("Content-Type", content_type);
  response.add_header("Content-Length", std::to_string(body_size));
  response.add_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  auto result = response.create_message(sp);
  copy_stream(sp, body, result.body()).execute([](const std::shared_ptr<std::exception> & ex) {
  });

  return result;
}

vds::http_message vds::http_response::file_response(const service_provider& sp, const const_data_buffer& body,
  const std::string& filename, const std::string& content_type, int result_code, const std::string& message) {

  http_response response(result_code, message);
  response.add_header("Content-Type", content_type);
  response.add_header("Content-Length", std::to_string(body.size()));
  response.add_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  auto result = response.create_message(sp);
  auto buffer = std::make_shared<const_data_buffer>(body);
  result.body()->write_async(buffer->data(), buffer->size())
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

  return result;

}

