#ifndef __VDS_HTTP_HTTP_FORM_PARSER_H_
#define __VDS_HTTP_HTTP_FORM_PARSER_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"
#include "http_multipart_reader.h"
#include "deflate.h"
#include "inflate.h"

namespace vds {
  namespace http {
    template <typename implementation_class>
    class form_parser : public std::enable_shared_from_this<implementation_class> {
    public:
      struct simple_field_info {
        std::string name;
        std::string value;
      };

      struct file_info {
        std::string name;
        std::string file_name;
        std::string mimetype;
        std::shared_ptr<stream_input_async<uint8_t>> stream;
      };
      //To override
//      void on_field(const vds::service_provider & sp, const simple_field_info & field) {
//      }
//
//      std::future<void> on_file(const vds::service_provider & sp, const file_info & file) {
//      }

      std::future<void> parse(
        const service_provider& sp,
        const http_message & message);


    private:
      class _form_parser : public std::enable_shared_from_this<_form_parser> {
      public:
        _form_parser(const std::shared_ptr<form_parser> & owner)
          : owner_(owner) {
        }

        std::future<void> read_part(
          const service_provider & sp,
          const http_message& part);

      private:
        std::shared_ptr<form_parser> owner_;
        uint8_t buffer_[1024];

        std::future<void> read_string_body(
            const service_provider & sp,
          const std::shared_ptr<std::string>& buffer,
          const http_message& part);

        std::future<void> read_file(
          const service_provider & sp,
          const std::shared_ptr<stream_output_async<uint8_t>> & buffer,
          const http_message& part);

          std::future<void> skip_part(
            const service_provider & sp,
          const vds::http_message& part);

      };
    };

    template <typename implementation_class>
    std::future<void> form_parser<implementation_class>::parse(const service_provider& sp, const http_message& message) {
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
            auto reader = std::make_shared<http_multipart_reader>("--" + boundary, [sp, task](const http_message& part)->std::future<void> {
              return task->read_part(sp, part);
            });

            co_return co_await reader->process(sp, message.body());
          }
          else {
            throw std::runtime_error("Invalid content type " + content_type);
          }
        }
        else {
          throw std::runtime_error("Invalid content type " + content_type);
        }
      }
      else {
        throw std::runtime_error("Invalid content type");
      }
    }

    template <typename implementation_class>
    std::future<void> form_parser<implementation_class>::_form_parser::read_part(const service_provider& sp,
      const http_message& part) {
      for(;;) {
        std::string content_disposition;
        if (part.get_header("Content-Disposition", content_disposition)) {
          std::list<std::string> items;
          for (;;) {
            auto p = content_disposition.find(';');
            if (std::string::npos == p) {
              vds::trim(content_disposition);
              items.push_back(content_disposition);
              break;
            } else {
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
                co_await
                this->read_string_body(sp, buffer, part);

                static_cast<implementation_class *>(this->owner_.get())->on_field(simple_field_info{name, *buffer});
                co_return;
              }

              std::string content_type;
              if (part.get_header("Content-Type", content_type)) {
                co_await
                static_cast<implementation_class *>(this->owner_.get())->on_file(
                    sp,
                    file_info{name, fname->second, content_type, part.body() });

                //if("application/x-zip-compressed" == content_type) {
                //  auto stream = std::make_shared<continuous_buffer<uint8_t>>(sp);
                //  auto buffer = std::make_shared<inflate_async>(*stream);
                //  static_cast<implementation_class *>(this->owner_.get())->on_file(file_info{ name, fname->second, stream });
                //  return this->read_file(buffer, part).then([stream](){});
                //}
              }
              throw std::runtime_error("Not implemented");
            }
            co_await this->skip_part(sp, part);
            co_return;
          }
        }

        auto readed = co_await
        part.body()->read_async(sp, this->buffer_, sizeof(this->buffer_));
        if (0 == readed) {
          co_return;
        }
      }
    }

    template <typename implementation_class>
    std::future<void> form_parser<implementation_class>::_form_parser::read_string_body(
        const service_provider & sp,
      const std::shared_ptr<std::string>& buffer, const http_message& part) {
      for(;;){
        auto readed = co_await part.body()->read_async(sp, this->buffer_, sizeof(this->buffer_));
        if (0 == readed) {
          co_return;
        }
        *buffer += std::string((const char *)this->buffer_, readed);
      }
    }

    template <typename implementation_class>
    std::future<void> form_parser<implementation_class>::_form_parser::read_file(
      const service_provider & sp,
      const std::shared_ptr<stream_output_async<uint8_t>>& buffer,
      const http_message& part) {
      for(;;){
        auto readed = co_await part.body()->read_async(sp, this->buffer_, sizeof(this->buffer_));

        if (0 == readed) {
          co_await buffer->write_async(sp, nullptr, 0);
          co_return;
        }

        co_await buffer->write_async(this->buffer_, readed);
      }
    }

    template <typename implementation_class>
    std::future<void> form_parser<implementation_class>::_form_parser::skip_part(
      const service_provider & sp,
      const vds::http_message& part) {
      for (;;) {
        auto readed = co_await part.body()->read_async(sp, this->buffer_, sizeof(this->buffer_));
        if (0 == readed) {
          co_return;
        }
      }
    }
  }
}

#endif // __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_

