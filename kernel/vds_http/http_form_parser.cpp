/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_form_parser.h"
#include "http_multipart_reader.h"

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::http::form_parser::parse(
  const http_message& message,
  lambda_holder_t<vds::async_task<vds::expected<void>>, std::shared_ptr<form_parser>> handler) {
  std::string content_type;
  if (message.get_header("Content-Type", content_type)) {
    static const char multipart_form_data[] = "multipart/form-data;";
    if (multipart_form_data == content_type.substr(0, sizeof(multipart_form_data) - 1)) {
      auto boundary = content_type.substr(sizeof(multipart_form_data) - 1);
      trim(boundary);
      static const char boundary_prefix[] = "boundary=";
      if (boundary_prefix == boundary.substr(0, sizeof(boundary_prefix) - 1)) {
        boundary.erase(0, sizeof(boundary_prefix) - 1);

        return http_multipart_reader::parse(
          this->sp_,
          "--" + boundary,
          [pthis = this->shared_from_this()](http_message && part) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
            return pthis->read_part(part);
          },
          [h = std::move(handler), pthis = this->shared_from_this()]() {
            return h(pthis);
          });
      }
      else {
        return vds::make_unexpected<std::runtime_error>("Invalid content type " + content_type);
      }
    }
    else {
      static const char form_urlencoded[] = "application/x-www-form-urlencoded;";
      if (form_urlencoded == content_type.substr(0, sizeof(form_urlencoded) - 1)) {
        return this->read_form_urlencoded(
          message,
          [h = std::move(handler), pthis = this->shared_from_this()]() {
          return h(pthis);
        });
      }
      else {
        return vds::make_unexpected<std::runtime_error>("Invalid content type " + content_type);
      }
    }
  }
  else {
    return vds::make_unexpected<std::runtime_error>("Invalid content type");
  }
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::http::form_parser::read_string_body(
  lambda_holder_t<async_task<expected<void>>, std::string &&> handler) {

  co_return std::make_shared<collect_data>([h = std::move(handler)](const_data_buffer && data){
    return h(std::string((const char *)data.data(), data.size()));
  });
}


vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::http::form_parser::read_part(
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

        auto fname = values.find("filename");
        if (values.end() == fname) {
          auto buffer = std::make_shared<std::string>();
          return this->read_string_body([name, pthis = this->shared_from_this()](std::string && body) {
            return pthis->on_field(simple_field_info{ name, url_encode::decode(body) });
          });
        }

        std::string content_type;
        if (part.get_header("Content-Type", content_type)) {
          return this->on_file(file_info{ name, url_encode::decode(fname->second), content_type, part.headers() });
          //if("application/x-zip-compressed" == content_type) {
          //  auto stream = std::make_shared<continuous_buffer<uint8_t>>(sp);
          //  auto buffer = std::make_shared<inflate_async>(*stream);
          //  static_cast<implementation_class *>(this->owner_.get())->on_file(file_info{ name, fname->second, stream });
          //  return this->read_file(buffer, part).then([stream](){});
          //}
        }

        return vds::make_unexpected<std::runtime_error>("Not implemented");
      }
    }
  }

  return std::shared_ptr<vds::stream_output_async<uint8_t>>();
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::http::form_parser::read_form_urlencoded(
  const http_message & /*part*/,
  lambda_holder_t<vds::async_task<vds::expected<void>>> final_handler)
{
    return read_string_body([pthis = this->shared_from_this(), h = std::move(final_handler)](std::string && buffer) -> async_task<expected<void>> {
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

          CHECK_EXPECTED_ASYNC(co_await pthis->on_field(simple_field_info{ item.substr(0, p),  url_encode::decode(value) }));
        }
      }

      CHECK_EXPECTED_ASYNC(co_await h());

      co_return expected<void>();
    });
}
