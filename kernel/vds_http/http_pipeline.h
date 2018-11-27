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

  class http_pipeline : public http_parser<http_pipeline>{
  public:
    http_pipeline(
      const std::shared_ptr<http_async_serializer> & output_stream,
      const std::function<vds::async_task<http_message>(const http_message message)> &message_callback)
      : http_parser([message_callback, this](
        const http_message message)->vds::async_task<void> {
      auto pthis = this->shared_from_this();
      auto response = co_await message_callback(message);
      if (nullptr != response.body()) {
        co_await this->output_stream_->write_async(response);
      }
    }),
      output_stream_(output_stream) {
    }

    vds::async_task<void> continue_read_data();

    vds::async_task<void> finish_message();

  private:
    std::shared_ptr<http_async_serializer> output_stream_;

    vds::async_task<void> send( const vds::http_message & message);
    vds::async_task<void> continue_send();
  };

  inline vds::async_task<void> http_pipeline::continue_read_data() {
    auto continue_message = http_response::status_response(100, "Continue");
    co_await this->output_stream_->write_async(continue_message);
  }

  inline vds::async_task<void> http_pipeline::finish_message() {
    co_return;
  }  
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
