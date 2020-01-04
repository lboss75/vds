#ifndef __VDS_HTTP__WEBSOCKET_CLIENT_H_
#define __VDS_HTTP__WEBSOCKET_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_request.h"
#include "http_response.h"
#include "async_mutex.h"
#include "http_client.h"

namespace vds {
	class websocket_client : public std::enable_shared_from_this<websocket_client> {
	public:

    async_task<expected<void>> open_connection(
      const service_provider* sp,
      const std::string& url);

    async_task<expected<void>> send_message(const std::string & message);
    expected<void> close();

    expected<void> set_text_handler(lambda_holder_t<async_task<expected<void>>, std::string /*message*/>&& handler);
    expected<void> set_binary_handler(lambda_holder_t<async_task<expected<void>>, const_data_buffer /*message*/>&& handler);
    expected<void> set_error_handler(lambda_holder_t<async_task<expected<void>>, std::shared_ptr<std::exception> /*error*/>&& handler);


  private:
    lambda_holder_t<async_task<expected<void>>, std::string /*message*/> text_handler_;
    lambda_holder_t<async_task<expected<void>>, const_data_buffer /*message*/> binary_handler_;
    lambda_holder_t<async_task<expected<void>>, std::shared_ptr<std::exception> /*error*/> error_handler_;

    std::shared_ptr<vds::http_client> client_;

    resizable_data_buffer send_buffer_;
    std::shared_ptr<stream_output_async<uint8_t>> body_stream_;
    async_result<expected<void>> body_result_;

    class input_stream : public stream_output_async<uint8_t> {
    public:
      input_stream(std::shared_ptr<websocket_client> owner);

      async_task<expected<void>> write_async(
        const uint8_t * data,
        size_t len) override;

    private:
      std::shared_ptr<websocket_client> owner_;
      resizable_data_buffer buffer_;
    };

    async_task<expected<void>> send_message(const uint8_t * message, uint64_t message_size, bool is_binary);

  };
}//vds

#endif //__VDS_HTTP__WEBSOCKET_CLIENT_H_
