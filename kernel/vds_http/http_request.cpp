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
