#ifndef __VDS_HTTP_HTTP_PARSER_H
#define __VDS_HTTP_HTTP_PARSER_H

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>
#include <list>
#include <optional>
#include <string.h>

#include "service_provider.h"
#include "http_message.h"

namespace vds {

  class http_parser : public stream_output_async<uint8_t> {
  public:
    http_parser(
		const service_provider * sp,
      lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> message_callback);

    ~http_parser();

    async_task<expected<void>> write_async(
      const uint8_t *data,
      size_t len) override;


    virtual async_task<expected<void>> continue_read_data() {
      co_return expected<void>();
    }

    virtual async_task<expected<void>> finish_message() {
      co_return expected<void>();
    }
    

  private:
	const service_provider * sp_;
  lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> message_callback_;

    bool eof_;
    std::string parse_buffer_;
    std::list<std::string> headers_;

    enum class StateEnum {
      STATE_PARSE_HEADER,
      STATE_PARSE_BODY,
      STATE_PARSE_SIZE,
      STATE_PARSE_FINISH_CHUNK,
      STATE_PARSE_FINISH_CHUNK1,//STATE_PARSE_FINISH_CHUNK + \r
    };
    StateEnum state_;

    size_t content_length_;
    bool chunked_encoding_;
    bool expect_100_;

    std::shared_ptr<stream_output_async<uint8_t>> current_message_;

    async_task<expected<void>> finish_body();

  };
}

#endif // __VDS_HTTP_HTTP_PARSER_H

