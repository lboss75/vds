#ifndef __VDS_HTTP_HTTP_FORM_PARSER_H_
#define __VDS_HTTP_HTTP_FORM_PARSER_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"
#include "http_multipart_reader.h"

namespace vds {
  namespace http {
    class form_parser : public std::enable_shared_from_this<form_parser> {
    public:

      form_parser(const service_provider * sp)
      : sp_(sp) {        
      }

      struct simple_field_info {
        std::string name;
        std::string value;
      };

      struct file_info {
        std::string name;
        std::string file_name;
        std::string mimetype;
        std::list<std::string> headers;
        std::shared_ptr<stream_input_async<uint8_t>> stream;
      };

      //To override
      virtual async_task<expected<void>> on_field( const simple_field_info & field) {
        co_return expected<void>();
      }

      virtual async_task<vds::expected<void>> on_file( const file_info & file) {
        co_return expected<void>();
      }

      vds::async_task<vds::expected<void>> parse(
        const http_message & message);

    protected:
      const service_provider * sp_;

    private:
      class _form_parser : public std::enable_shared_from_this<_form_parser> {
      public:
        _form_parser(const std::shared_ptr<form_parser> & owner)
          : owner_(owner) {
        }

        vds::async_task<vds::expected<void>> read_part(
          
          const http_message& part);

      private:
        std::shared_ptr<form_parser> owner_;
        uint8_t buffer_[1024];

        vds::async_task<vds::expected<void>> read_string_body(
            
          const std::shared_ptr<std::string>& buffer,
          const http_message& part);

        vds::async_task<vds::expected<void>> read_file(
          
          const std::shared_ptr<stream_output_async<uint8_t>> & buffer,
          const http_message& part);

          vds::async_task<vds::expected<void>> skip_part(
            
          const vds::http_message& part);

      };
    };

  }
}

#endif // __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_

