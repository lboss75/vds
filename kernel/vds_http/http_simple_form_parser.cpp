/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_simple_form_parser.h"
#include "http_multipart_reader.h"

vds::async_task<vds::expected<void>> vds::http::simple_form_parser::parse( const http_message& message) {
  std::string content_type;
  if (message.get_header("Content-Type", content_type)) {
    static const char multipart_form_data[] = "multipart/form-data;";
    if (multipart_form_data == content_type.substr(0, sizeof(multipart_form_data) - 1)) {
      auto boundary = content_type.substr(sizeof(multipart_form_data) - 1);
      trim(boundary);
      static const char boundary_prefix[] = "boundary=";
      if (boundary_prefix == boundary.substr(0, sizeof(boundary_prefix) - 1)) {
        boundary.erase(0, sizeof(boundary_prefix) - 1);

        auto task = std::make_shared<form_parser>(this->shared_from_this());
        auto reader = std::make_shared<http_multipart_reader>(this->sp_, "--" + boundary, [task](const http_message& part)->vds::async_task<vds::expected<void>> {
          CHECK_EXPECTED_ASYNC(co_await task->read_part(part));
          co_return expected<void>();
        });
        CHECK_EXPECTED_ASYNC(co_await reader->process(message.body()));
        co_return expected<void>();
      }
      else {
        co_return vds::make_unexpected<std::runtime_error>("Invalid content type " + content_type);
      }
    }
    else {
      static const char form_urlencoded[] = "application/x-www-form-urlencoded;";
      if (form_urlencoded == content_type.substr(0, sizeof(form_urlencoded) - 1)) {
        auto task = std::make_shared<form_parser>(this->shared_from_this());
        CHECK_EXPECTED_ASYNC(co_await task->read_form_urlencoded(message));
        co_return expected<void>();
      }
      else {
        co_return vds::make_unexpected<std::runtime_error>("Invalid content type " + content_type);
      }
    }
  }
  else {
    co_return vds::make_unexpected<std::runtime_error>("Invalid content type");
  }

  co_return expected<void>();
}

vds::async_task<vds::expected<std::string>> vds::http::simple_form_parser::form_parser::read_string_body(  
  const http_message& part) {

  std::string buffer;
  for (;;) {
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await part.body()->read_async(this->buffer_, sizeof(this->buffer_)));
    if (0 == readed) {
      co_return buffer;
    }
    buffer += std::string((const char *)this->buffer_, readed);
  }
}

vds::http::simple_form_parser::form_parser::form_parser(const std::shared_ptr<simple_form_parser>& owner)
: owner_(owner) {
}

vds::async_task<vds::expected<void>> vds::http::simple_form_parser::form_parser::read_part(
  const http_message& part) {
  for (;;) {
    std::string content_disposition;
    if (part.get_header("Content-Disposition", content_disposition)) {
      std::list<std::string> items;
      for (;;) {
        auto p = content_disposition.find(';');
        if (std::string::npos == p) {
          vds::trim(content_disposition);
          items.push_back(content_disposition);
          break;
        }
        else {
          items.push_back(vds::trim_copy(content_disposition.substr(0, p)));
          content_disposition.erase(0, p + 1);
        }
      }

      if (!items.empty() && "form-data" == *items.begin()) {
        std::map<std::string, std::string> values;
        for (const auto & item : items) {
          auto p = item.find('=');
          if (std::string::npos != p) {
            auto value = item.substr(p + 1);
            if (!value.empty()
              && value[0] == '\"'
              && value[value.length() - 1] == '\"') {
              value.erase(0, 1);
              value.erase(value.length() - 1, 1);
            }

            values[item.substr(0, p)] = value;
          }
        }

        auto pname = values.find("name");
        if (values.end() != pname) {
          auto name = pname->second;
          GET_EXPECTED_VALUE_ASYNC(this->owner_->values_[name], co_await this->read_string_body(part));
        }

        co_return co_await this->skip_part(part);
      }
    }

    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await part.body()->read_async(this->buffer_, sizeof(this->buffer_)));
    if (0 == readed) {
      co_return expected<void>();
    }
  }
}

vds::async_task<vds::expected<void>> vds::http::simple_form_parser::form_parser::read_form_urlencoded(
  const http_message& message) {
  std::string buffer;
  GET_EXPECTED_VALUE_ASYNC(buffer, co_await this->read_string_body(message));
  auto items = split_string(buffer, '&', true);
  std::map<std::string, std::string> values;
  for (const auto & item : items) {
    auto p = item.find('=');
    if (std::string::npos != p) {
      auto value = item.substr(p + 1);
      if (!value.empty()
        && value[0] == '\"'
        && value[value.length() - 1] == '\"') {
        value.erase(0, 1);
        value.erase(value.length() - 1, 1);
      }

      this->owner_->values_[item.substr(0, p)] = url_encode::decode(value);
    }
  }

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http::simple_form_parser::form_parser::skip_part(
  
  const vds::http_message& part) {

  for (;;) {
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await part.body()->read_async(this->buffer_, sizeof(this->buffer_)));
    if (0 == readed) {
      co_return expected<void>();
    }
  }
}

