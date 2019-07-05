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
  class tcp_network_socket;

  class http_client : public std::enable_shared_from_this<http_client>
  {
  public:
    expected<void> start(
		  const service_provider * sp,
      const std::shared_ptr<tcp_network_socket> & s,
      const std::shared_ptr<stream_output_async<uint8_t>> & output_stream);

    async_task<expected<void>> send(
      http_message message,
      lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> response_handler);

    async_task<expected<const_data_buffer>> send(
      http_message message,
      lambda_holder_t<async_task<expected<void>>, http_message> response_handler);

    async_task<expected<void>> send_headers(
      http_message message,
      lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> response_handler,
      lambda_holder_t<async_task<expected<void>>, std::shared_ptr<stream_output_async<uint8_t>>> body_callback);

    async_task<expected<const_data_buffer>> send_headers(
      http_message message,
      lambda_holder_t<async_task<expected<void>>, http_message> response_handler,
      lambda_holder_t<async_task<expected<void>>, std::shared_ptr<stream_output_async<uint8_t>>> body_callback);

    async_task<expected<void>> close();


  private:
    class client_pipeline : public http_parser {
    public:
      client_pipeline(
		    const service_provider * sp,
        std::shared_ptr<http_client> owner,
        lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> message_callback);


      async_task<expected<void>> finish_message() override;
      async_task<expected<void>> before_close() override;

    private:
      std::shared_ptr<http_client> owner_;
    };
    std::shared_ptr<http_async_serializer> output_;

    lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> response_handler_;
    lambda_holder_t<async_task<expected<void>>, expected<void>> final_handler_;
  };
}

#endif // __VDS_HTTP_HTTP_CLIENT_H_
