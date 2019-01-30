/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_form_parser.h"
#include "http_multipart_reader.h"

vds::async_task<vds::expected<void>> vds::http::form_parser::parse(const http_message& message) {
  std::string content_type;
  if (message.get_header("Content-Type", content_type)) {
    static const char multipart_form_data[] = "multipart/form-data;";
    if (multipart_form_data == content_type.substr(0, sizeof(multipart_form_data) - 1)) {
      auto boundary = content_type.substr(sizeof(multipart_form_data) - 1);
      trim(boundary);
      static const char boundary_prefix[] = "boundary=";
      if (boundary_prefix == boundary.substr(0, sizeof(boundary_prefix) - 1)) {
        boundary.erase(0, sizeof(boundary_prefix) - 1);

        auto task = std::make_shared<_form_parser>(this->shared_from_this());
        auto reader = std::make_shared<http_multipart_reader>(this->sp_, "--" + boundary, [task](const http_message& part)->vds::async_task<vds::expected<void>> {
          return task->read_part(part);
        });

        co_return co_await reader->process(message.body());
      }
      else {
        co_return vds::make_unexpected<std::runtime_error>("Invalid content type " + content_type);
      }
    }
    else {
      co_return vds::make_unexpected<std::runtime_error>("Invalid content type " + content_type);
    }
  }
  else {
    co_return vds::make_unexpected<std::runtime_error>("Invalid content type");
  }

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http::form_parser::_form_parser::read_part(
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
        for (const auto &item : items) {
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

          auto fname = values.find("filename");
          if (values.end() == fname) {
            auto buffer = std::make_shared<std::string>();
            CHECK_EXPECTED_ASYNC(co_await this->read_string_body(buffer, part));
            CHECK_EXPECTED_ASYNC(co_await this->owner_->on_field(simple_field_info{
              name,
              url_encode::decode(*buffer) }));
            co_return expected<void>();
          }

          std::string content_type;
          if (part.get_header("Content-Type", content_type)) {
            CHECK_EXPECTED_ASYNC(co_await this->owner_->on_file(
              file_info{ name, fname->second, content_type, part.body() }));
            co_return  expected<void>();
            //if("application/x-zip-compressed" == content_type) {
            //  auto stream = std::make_shared<continuous_buffer<uint8_t>>(sp);
            //  auto buffer = std::make_shared<inflate_async>(*stream);
            //  static_cast<implementation_class *>(this->owner_.get())->on_file(file_info{ name, fname->second, stream });
            //  return this->read_file(buffer, part).then([stream](){});
            //}
          }
          co_return vds::make_unexpected<std::runtime_error>("Not implemented");
        }
        CHECK_EXPECTED_ASYNC(co_await this->skip_part(part));
        co_return expected<void>();
      }
    }

    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await part.body()->read_async(this->buffer_, sizeof(this->buffer_)));
    if (0 == readed) {
      co_return expected<void>();
    }
  }
}

vds::async_task<vds::expected<void>> vds::http::form_parser::_form_parser::read_string_body(

  const std::shared_ptr<std::string>& buffer, const http_message& part) {
  for (;;) {
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await part.body()->read_async(this->buffer_, sizeof(this->buffer_)));
    if (0 == readed) {
      co_return expected<void>();
    }
    *buffer += std::string((const char *)this->buffer_, readed);
  }
}

vds::async_task<vds::expected<void>> vds::http::form_parser::_form_parser::read_file(

  const std::shared_ptr<stream_output_async<uint8_t>>& buffer,
  const http_message& part) {
  for (;;) {
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await part.body()->read_async(this->buffer_, sizeof(this->buffer_)));

    if (0 == readed) {
      CHECK_EXPECTED_ASYNC(co_await buffer->write_async(nullptr, 0));
      co_return expected<void>();
    }

    CHECK_EXPECTED_ASYNC(co_await buffer->write_async(this->buffer_, readed));
  }
}

vds::async_task<vds::expected<void>> vds::http::form_parser::_form_parser::skip_part(

  const vds::http_message& part) {
  for (;;) {
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await part.body()->read_async(this->buffer_, sizeof(this->buffer_)));
    if (0 == readed) {
      co_return expected<void>();
    }
  }
}
