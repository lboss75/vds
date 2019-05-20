#ifndef __VDS_HTTP_HTTP_CLIENT_H_
#define __VDS_HTTP_HTTP_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "async_buffer.h"
#include "http_parser.h"

namespace vds {
  class http_message;
  class http_async_serializer;

  class http_client : public std::enable_shared_from_this<http_client>
  {
  public:
    async_task<expected<void>> start(
		  const service_provider * sp,
      const std::shared_ptr<stream_input_async<uint8_t>> & input_stream,
      const std::shared_ptr<stream_output_async<uint8_t>> & output_stream);

    async_task<expected<void>> send(
      http_message && message,
      lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message &&> && response_handler);

    async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> send_headers(
      http_message && message,
      lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message &&> && response_handler);

    async_task<expected<void>> close();


  private:
    class client_pipeline : public http_parser {
    public:
      client_pipeline(
		    const service_provider * sp,
        lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message && > message_callback);

    };
    std::shared_ptr<http_async_serializer> output_;
    std::shared_ptr<client_pipeline> pipeline_;

    lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message && > response_handler_;
  };
}

#endif // __VDS_HTTP_HTTP_CLIENT_H_
