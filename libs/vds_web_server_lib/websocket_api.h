#ifndef __VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_
#define __VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_request.h"
#include "http_response.h"

namespace vds {
	class websocket_api : public std::enable_shared_from_this<websocket_api> {
	public:
		websocket_api(const service_provider * sp);

		static vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>> open_connection(
			const vds::service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
			const http_message & /*message*/);

	private:
		std::shared_ptr<stream_input_async<uint8_t>> start(const http_message & request);

		async_task<expected<void>> start_parse(
			std::shared_ptr<json_parser> parser,
			std::shared_ptr<stream_input_async<uint8_t>> request_stream);

		const service_provider * sp_;
		vds::async_task<vds::expected<void>> task_;


		async_task<expected<std::shared_ptr<json_value>>> login(
			const std::string & username,
			const std::string & password_hash);

	};

}//vds

#endif //__VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_
