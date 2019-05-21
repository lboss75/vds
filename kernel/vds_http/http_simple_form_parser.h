#ifndef __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_
#define __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"
#include "http_form_parser.h"

namespace vds {
  namespace http {
    class simple_form_parser : public form_parser {
    public:

      simple_form_parser(const service_provider * sp)
        : form_parser(sp) {
      }

      const std::map<std::string, std::string> & values() const {
        return this->values_;
      }

      async_task<expected<void>> on_field(const field_info & field) override;
      async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> on_file(const file_info & file) override;

    private:
      std::map<std::string, std::string> values_;
    };
  }
}

#endif // __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_

