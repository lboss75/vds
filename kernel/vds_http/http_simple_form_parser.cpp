/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_simple_form_parser.h"
#include "http_multipart_reader.h"

std::future<void> vds::http::simple_form_parser::parse(const service_provider& sp, const http_message& message) {
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
        auto reader = std::make_shared<http_multipart_reader>(sp, "--" + boundary, [sp, task](const http_message& part)->std::future<void> {
          return task->read_part(sp, part);
        });

        return reader->process(sp, message);
      }
      else {
        throw std::runtime_error("Invalid content type " + content_type);
      }
    }
    else {
      static const char form_urlencoded[] = "application/x-www-form-urlencoded;";
      if (form_urlencoded == content_type.substr(0, sizeof(form_urlencoded) - 1)) {
        auto task = std::make_shared<form_parser>(this->shared_from_this());
        return task->read_form_urlencoded(sp, message);
      }
      else {
        throw std::runtime_error("Invalid content type " + content_type);
      }
    }
  }
  else {
    throw std::runtime_error("Invalid content type");
  }
}

std::future<void> vds::http::simple_form_parser::form_parser::read_string_body(
  const std::shared_ptr<std::string>& buffer, const http_message& part) {
  return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
    .then([pthis = this->shared_from_this(), buffer, part](size_t readed)->std::future<void> {
    if (0 == readed) {
      return std::future<void>::empty();
    }
    *buffer += std::string((const char *)pthis->buffer_, readed);
    return pthis->read_string_body(buffer, part);
  });
}

vds::http::simple_form_parser::form_parser::form_parser(const std::shared_ptr<simple_form_parser>& owner)
: owner_(owner) {
}

std::future<void> vds::http::simple_form_parser::form_parser::read_part(const service_provider& sp,
  const http_message& part) {
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
        auto buffer = std::make_shared<std::string>();
        return this->read_string_body(buffer, part).then([name, buffer, pthis = this->shared_from_this()]() {
          pthis->owner_->values_[name] = *buffer;
        });
      }
      return this->skip_part(part);
    }
  }

  return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
    .then([pthis = this->shared_from_this(), sp, part](size_t readed)->std::future<void> {
    if (0 == readed) {
      return std::future<void>::empty();
    }

    return pthis->read_part(sp, part);
  });
}

std::future<void> vds::http::simple_form_parser::form_parser::read_form_urlencoded(const service_provider& sp,
  const http_message& message) {
  auto buffer = std::make_shared<std::string>();
  return this->read_string_body(buffer, message)
  .then([buffer, pthis = this->shared_from_this()]() {
    auto items = split_string(*buffer, '&', true);
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

        pthis->owner_->values_[item.substr(0, p)] = http_parser::url_unescape(value);
      }
    }
  });
}

std::future<void> vds::http::simple_form_parser::form_parser::skip_part(const vds::http_message& part) {
  return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
    .then([pthis = this->shared_from_this(), part](size_t readed)->std::future<void> {
    if (0 == readed) {
      return std::future<void>::empty();
    }

    return pthis->skip_part(part);
  });
}

