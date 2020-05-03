#ifndef __VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_
#define __VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_request.h"
#include "http_response.h"

namespace vds {
  class websocket_output;

	class websocket_api : public std::enable_shared_from_this<websocket_api> {
	public:
    websocket_api();

		static vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>> open_connection(
			const vds::service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
			const http_message & /*message*/);

	private:

    class subscribe_handler : public std::enable_shared_from_this<subscribe_handler>
    {
    public:
      subscribe_handler(
        std::string cb,
        const_data_buffer channel_id);

      async_task<expected<void>> process(
        const vds::service_provider * sp,
        std::weak_ptr<websocket_output> output_stream);

    private:
      std::string cb_;
      const_data_buffer channel_id_;

      int last_id_;
    };

    timer subscribe_timer_;
    std::list<std::shared_ptr<subscribe_handler>> subscribe_handlers_;

    async_task<expected<std::shared_ptr<json_value>>> process_message(
      const vds::service_provider * sp,
      std::shared_ptr<websocket_output> output_stream,
      const std::shared_ptr<json_value> & message,
      std::list<lambda_holder_t<async_task<expected<void>>>> & post_tasks);

    async_task<expected<std::shared_ptr<json_value>>> process_message(
      const vds::service_provider * sp,
      std::shared_ptr<websocket_output> output_stream,
      int id,
      const std::shared_ptr<json_object> & request,
      std::list<lambda_holder_t<async_task<expected<void>>>> & post_tasks);

		async_task<expected<void>> login(
      const vds::service_provider * sp,
      std::shared_ptr<json_object> result,
      std::string login);

    async_task<expected<void>> log_head(
      const vds::service_provider* sp,
      std::shared_ptr<json_object> result);

    async_task<expected<void>> upload(
      const vds::service_provider * sp,
      std::shared_ptr<json_object> result,
      const_data_buffer body);

    std::shared_ptr<subscribe_handler> subscribe_channel(
      const vds::service_provider * sp,
      std::shared_ptr<json_object> result,
      std::string cb,
      const_data_buffer channel_id);

    expected<void> start_timer(
      const vds::service_provider * sp,
      std::shared_ptr<websocket_output> output_stream);

    async_task<expected<void>> devices(
      const vds::service_provider * sp,
      std::shared_ptr<json_object> result,
      const_data_buffer owner_id);

    async_task<expected<void>> get_channel_messages(
      const vds::service_provider * sp,
      std::shared_ptr<json_object> result,
      const_data_buffer channel_id,
      int64_t last_id,
      uint32_t limit);

    async_task<expected<void>> allocate_storage(
      const vds::service_provider* sp,
      std::shared_ptr<json_object> result,
      asymmetric_public_key user_public_key,
      foldername folder,
      long size);

    async_task<expected<void>> looking_block(
      const vds::service_provider* sp,
      std::shared_ptr<json_object> result,
      const_data_buffer data_hash);

    async_task<expected<void>> download(
      const vds::service_provider * sp,
      std::shared_ptr<json_object> result,
      std::vector<const_data_buffer> object_ids);


    async_task<expected<void>> broadcast(
      const vds::service_provider * sp,
      std::shared_ptr<json_object> result,
      const_data_buffer body);

    async_task<expected<void>> get_balance(
        const vds::service_provider* sp,
        std::shared_ptr<json_object> result,
        const_data_buffer wallet_id);
  };

}//vds

#endif //__VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_
