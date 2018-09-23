#ifndef __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_
#define __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"

namespace vds {
  namespace http {
    class simple_form_parser : public std::enable_shared_from_this<simple_form_parser> {
    public:
      vds::async_task<void> parse(
        const service_provider& sp,
        const http_message& message);

      const std::map<std::string, std::string> & values() const {
        return this->values_;
      }

    private:
      std::map<std::string, std::string> values_;

      class form_parser : public std::enable_shared_from_this<form_parser> {
      public:
        form_parser(const std::shared_ptr<simple_form_parser> & owner);

        vds::async_task<void> read_part(
          const service_provider & sp,
          const http_message& part);

        vds::async_task<void> read_form_urlencoded(
          const service_provider& sp, 
          const http_message& message);

      private:
        std::shared_ptr<simple_form_parser> owner_;
        uint8_t buffer_[1024];

        vds::async_task<std::string> read_string_body(
          const service_provider & sp,
          const http_message& part);

        vds::async_task<void> skip_part(
          const service_provider & sp,
          const vds::http_message& part);

      };
    };
  }
}

#endif // __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_

