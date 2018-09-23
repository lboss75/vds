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
        std::shared_ptr<continuous_buffer<uint8_t>> stream;
      };
      //To override
//      void on_field(const simple_field_info & field) {
//      }
//
//      vds::async_task<void> on_file(const file_info & file) {
//      }

      vds::async_task<void> parse(
        const service_provider& sp,
        const http_message & message);


    private:
      class _form_parser : public std::enable_shared_from_this<_form_parser> {
      public:
        _form_parser(const std::shared_ptr<form_parser> & owner)
          : owner_(owner) {
        }

        vds::async_task<void> read_part(
          const service_provider & sp,
          const http_message& part);

      private:
        std::shared_ptr<form_parser> owner_;
        uint8_t buffer_[1024];

        vds::async_task<void> read_string_body(
          const std::shared_ptr<std::string>& buffer,
          const http_message& part);

        vds::async_task<void> read_file(
          const std::shared_ptr<stream_async<uint8_t>> & buffer,
          const http_message& part);

          vds::async_task<void> skip_part(
          const vds::http_message& part);

      };
    };

    template <typename implementation_class>
    vds::async_task<void> form_parser<implementation_class>::parse(const service_provider& sp, const http_message& message) {
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
            auto reader = std::make_shared<http_multipart_reader>(sp, "--" + boundary, [sp, task](const http_message& part)->vds::async_task<void> {
              return task->read_part(sp, part);
            });

            return reader->process(sp, message);
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
    vds::async_task<void> form_parser<implementation_class>::_form_parser::read_part(const service_provider& sp,
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
              return this->read_string_body(buffer, part).then([name, buffer, pthis = this->shared_from_this()]() {
                static_cast<implementation_class *>(pthis->owner_.get())->on_field(simple_field_info{ name, *buffer });
              });
            }

            std::string content_type;
            if (part.get_header("Content-Type", content_type)) {
              const auto stream = std::make_shared<continuous_buffer<uint8_t>>(sp);
              return async_series(
                static_cast<implementation_class *>(this->owner_.get())->on_file(file_info{ name, fname->second, content_type, stream }),
                this->read_file(stream, part));
              //if("application/x-zip-compressed" == content_type) {
              //  auto stream = std::make_shared<continuous_buffer<uint8_t>>(sp);
              //  auto buffer = std::make_shared<inflate_async>(*stream);
              //  static_cast<implementation_class *>(this->owner_.get())->on_file(file_info{ name, fname->second, stream });
              //  return this->read_file(buffer, part).then([stream](){});
              //}
            }
            throw std::runtime_error("Not implemented");
          }
          return this->skip_part(part);
        }
      }

      return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
        .then([pthis = this->shared_from_this(), sp, part](size_t readed)->vds::async_task<void> {
        if (0 == readed) {
          return vds::async_task<void>::empty();
        }

        return pthis->read_part(sp, part);
      });
    }

    template <typename implementation_class>
    vds::async_task<void> form_parser<implementation_class>::_form_parser::read_string_body(
      const std::shared_ptr<std::string>& buffer, const http_message& part) {
      return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
        .then([pthis = this->shared_from_this(), buffer, part](size_t readed)->vds::async_task<void> {
        if (0 == readed) {
          return vds::async_task<void>::empty();
        }
        *buffer += std::string((const char *)pthis->buffer_, readed);
        return pthis->read_string_body(buffer, part);
      });
    }

    template <typename implementation_class>
    vds::async_task<void> form_parser<implementation_class>::_form_parser::read_file(
      const std::shared_ptr<stream_async<uint8_t>>& buffer, const http_message& part) {
      return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
        .then([pthis = this->shared_from_this(), buffer, part](size_t readed)->vds::async_task<void> {
        if (0 == readed) {
          return buffer->write_async(nullptr, 0);
        }

        return buffer->write_async(pthis->buffer_, readed)
          .then([pthis, buffer, part]() {
          return pthis->read_file(buffer, part);
        });
      });
    }

    template <typename implementation_class>
    vds::async_task<void> form_parser<implementation_class>::_form_parser::skip_part(const vds::http_message& part) {
      return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
        .then([pthis = this->shared_from_this(), part](size_t readed)->vds::async_task<void> {
        if (0 == readed) {
          return vds::async_task<void>::empty();
        }

        return pthis->skip_part(part);
      });
    }
  }
}

#endif // __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_

