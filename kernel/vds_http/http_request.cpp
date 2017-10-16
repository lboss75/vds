/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_request.h"

vds::http_request::http_request(const std::shared_ptr<http_message>& message)
  : message_(message)
{
  if (0 == message->headers().size()) {
    throw std::runtime_error("Invalid HTTP Request");
  }

  int index = 0;
  for (auto ch : *(message->headers().begin())) {
    if (isspace(ch)) {
      if (index < 2) {
        ++index;
      }
      else {
        this->agent_ += ch;
      }
    }
    else {
      switch (index) {
      case 0:
        this->method_ += ch;
        break;

      case 1:
        this->url_ += ch;
        break;

      case 2:
        this->agent_ += ch;
        break;
      }
    }
  }

  this->parse_parameters();
}

void vds::http_request::parse_parameters()
{
  this->parameters_.clear();

  auto p = strchr(this->url_.c_str(), '?');
  if (nullptr != p) {
    this->parameters_.push_back(p + 1);
    this->url_.erase(p - this->url_.c_str());
  }
}

std::shared_ptr<vds::http_message> vds::http_request::simple_request(
  const service_provider & sp,
  const std::string& method,
  const std::string& url,
  const std::string& body)
{
  std::list<std::string> headers;
  headers.push_back(method + " " + url + " HTTP/1.0");
  headers.push_back("Content-Type: text/html; charset=utf-8");
  headers.push_back("Content-Length:" + std::to_string(body.length()));
  
  auto result = std::make_shared<http_message>(headers);
  auto buffer = std::make_shared<std::string>(body);

  result->body()->write_all_async(sp, (const uint8_t *)buffer->c_str(), buffer->length())
    .wait(
    [buffer, result, sp]() {
      result->body()->write_all_async(sp, nullptr, 0).wait(
        []() {},
        [sp](const std::shared_ptr<std::exception> & ex) {
          sp.unhandled_exception(ex);
        });
      },
      [buffer, sp](const std::shared_ptr<std::exception> & ex) {
        sp.unhandled_exception(ex);
      });

  return result;
}
