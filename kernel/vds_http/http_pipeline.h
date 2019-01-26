#ifndef __VDS_HTTP_HTTP_PIPELINE_H_
#define __VDS_HTTP_HTTP_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <queue>
#include "http_parser.h"
#include "http_serializer.h"
#include "http_response.h"

namespace vds {

  class http_pipeline : public http_parser {
  public:
    http_pipeline(
      const std::shared_ptr<http_async_serializer> & output_stream,
      const std::function<vds::async_task<vds::expected<http_message>>(http_message message)> &message_callback)
      : http_parser([message_callback, this](
        const http_message message)->vds::async_task<vds::expected<void>> {
              auto pthis = this->shared_from_this();
              //std::string keep_alive_header;
              //bool keep_alive = message.get_header("Connection", keep_alive_header) && keep_alive_header == "Keep-Alive";
              GET_EXPECTED_ASYNC(response, co_await message_callback(message));
              if (response) {
                CHECK_EXPECTED_ASYNC(co_await static_cast<http_pipeline *>(pthis.get())->output_stream_->write_async(response));
              }
          co_return expected<void>();
      }),
      output_stream_(output_stream) {
    }

    vds::async_task<vds::expected<void>> continue_read_data() override;

    vds::async_task<vds::expected<void>> finish_message() override;

  private:
    std::shared_ptr<http_async_serializer> output_stream_;

    vds::async_task<vds::expected<void>> send( const vds::http_message & message);
    vds::async_task<vds::expected<void>> continue_send();
  };

  inline vds::async_task<vds::expected<void>> http_pipeline::continue_read_data() {
    auto continue_message = http_response::status_response(100, "Continue");
    return this->output_stream_->write_async(continue_message);
  }

  inline vds::async_task<vds::expected<void>> http_pipeline::finish_message() {
    co_return expected<void>();
  }  
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
