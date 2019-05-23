#ifndef __VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_
#define __VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_request.h"
#include "http_response.h"

namespace vds {
	class websocket_api {
	public:
		static vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>> open_connection(
			const vds::service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
			const http_message & /*message*/);

	private:
    static async_task<expected<std::shared_ptr<json_value>>> process_message(
      const vds::service_provider * sp,
      const std::shared_ptr<json_value> & message);

    static async_task<expected<std::shared_ptr<json_value>>> process_message(
      const vds::service_provider * sp,
      int id,
      const std::shared_ptr<json_object> & request);

		static async_task<expected<std::shared_ptr<json_value>>> login(
      const vds::service_provider * sp,
      const std::string & username,
			const std::string & password_hash);
	};

}//vds

#endif //__VDS_WEB_SERVER_LIB_WEBSOCKET_API_H_
