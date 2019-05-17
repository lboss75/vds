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
    vds::async_task<vds::expected<void>> start(
		const service_provider * sp,
      const std::shared_ptr<vds::stream_input_async<uint8_t>> & input_stream,
      const std::shared_ptr<vds::stream_output_async<uint8_t>> & output_stream);

    vds::async_task<vds::expected<void>> send(
      vds::http_message message,
      const std::function<vds::async_task<vds::expected<void>>(vds::http_message response)> & response_handler);

  private:
    class client_pipeline : public http_parser {
    public:
      client_pipeline(
		    const service_provider * sp,
        lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message && > message_callback);

    };
    std::shared_ptr<http_async_serializer> output_;
    std::shared_ptr<client_pipeline> pipeline_;

    std::shared_ptr<async_result<vds::expected<void>>> result_;
    std::function<vds::async_task<vds::expected<void>>(vds::http_message response)> response_handler_;
  };
}

#endif // __VDS_HTTP_HTTP_CLIENT_H_
