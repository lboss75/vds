#ifndef __VDS_HTTP__WEBSOCKET_H_
#define __VDS_HTTP__WEBSOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_request.h"
#include "http_response.h"

namespace vds {

	class websocket_output : public std::enable_shared_from_this<websocket_output>
	{
	public:
		async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> start(size_t message_size, bool is_binary);
	};

	class websocket : public std::enable_shared_from_this<websocket> {
	public:
		enum class message_type {
			close,
			text,
			binary
		};

		struct message {
			message_type type;
			std::shared_ptr<stream_input_async<uint8_t>> body;
		};

		websocket(
			const service_provider * sp,
			const std::function<async_task<expected<void>> (message msg, std::shared_ptr<websocket_output> & out)> & handler);

		static vds::async_task<vds::expected<http_message>> open_connection(
			const vds::service_provider * sp,
			const http_request & message);

	private:
		std::shared_ptr<stream_input_async<uint8_t>> start(const http_request & request);

		async_task<expected<void>> start_read();

		const service_provider * sp_;
		vds::async_task<vds::expected<void>> task_;
		std::function<async_task<expected<void>>(message msg, std::shared_ptr<websocket_output> & out)> handler_;

		//Read state
		enum class read_state_t {
			BOF,
			TEXT,
			BINARY,
			CLOSED
		};

		read_state_t read_state_ = read_state_t::BOF;

		uint8_t buffer_[1024];
		size_t readed_ = 0;

		bool fin_;
		bool RSV1_;
		bool RSV2_;
		bool RSV3_;
		unsigned int Opcode_;

		bool has_mask_;
		uint64_t payloadLength_;
		size_t offset_;

		uint8_t mask_[4];
		int mask_index_;

		std::shared_ptr<stream_input_async<uint8_t>> input_stream_;

		async_task<expected<message_type>> next_message();
		async_task<expected<bool>> read_minimal(size_t min_size);
		async_task<expected<size_t>> read(uint8_t * buffer, size_t buffer_size);


		class websocket_stream : public stream_input_async<uint8_t>
		{
		public:
			websocket_stream(std::shared_ptr<websocket> owner)
				: owner_(owner) {
			}

			async_task<expected<size_t>> read_async(
				uint8_t * buffer,
				size_t len) override {
				return this->owner_->read(buffer, len);
			}

		private:
			std::shared_ptr<websocket> owner_;
		};

	};

}//vds

#endif //__VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_
