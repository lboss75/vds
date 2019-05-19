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
      };

      //To override
      virtual async_task<expected<void>> on_field( const simple_field_info & /*field*/) {
        return expected<void>();
      }

      virtual vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> on_file( const file_info & /*file*/) {
        return std::shared_ptr<vds::stream_output_async<uint8_t>>();
      }

      vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> parse(
        const http_message & message,
        lambda_holder_t<vds::async_task<vds::expected<void>>> final_handler);


    protected:
      const service_provider * sp_;

    private:
      vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> read_part(
        const http_message & part);

      vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> read_form_urlencoded(
        const http_message & part,
        lambda_holder_t<vds::async_task<vds::expected<void>>> final_handler);


      static vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> read_string_body(
        lambda_holder_t<async_task<expected<void>>, std::string &&> handler);
    };

  }
}

#endif // __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_

