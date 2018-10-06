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
      const service_provider *sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const std::function<std::future<http_message>(const http_message message)> &message_callback)
      : http_parser([sp, message_callback, this](
        const http_message message)->std::future<void> {
      auto pthis = this->shared_from_this();
      auto response = co_await message_callback(message);
      if (nullptr != response.body()) {
        co_await this->output_stream_->write_async(
          sp, response);
      }
    }),
      output_stream_(output_stream) {
    }

    std::future<void> continue_read_data(const service_provider * sp);

    std::future<void> finish_message(const service_provider * sp);

  private:
    std::shared_ptr<http_async_serializer> output_stream_;

    std::future<void> send(const vds::service_provider * sp, const vds::http_message & message);
    std::future<void> continue_send(const vds::service_provider * sp);
  };

  inline std::future<void> http_pipeline::continue_read_data(const service_provider * sp) {
    auto continue_message = http_response::status_response(100, "Continue");
    co_await this->output_stream_->write_async(
      sp, continue_message);
  }

  inline std::future<void> http_pipeline::finish_message(const service_provider * /*sp*/) {
    co_return;
  }  
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
