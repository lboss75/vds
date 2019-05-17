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

  class http_pipeline : public http_parser {
  public:
    http_pipeline(
	  const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message && > message_callback)
      : http_parser(sp, message_callback), output_stream_(output_stream) {
    }

    vds::async_task<vds::expected<void>> continue_read_data() override;

    vds::async_task<vds::expected<void>> finish_message() override;

  private:
    std::shared_ptr<http_async_serializer> output_stream_;

    vds::async_task<vds::expected<void>> send( const vds::http_message & message);
    vds::async_task<vds::expected<void>> continue_send();
  };

  inline vds::async_task<vds::expected<void>> http_pipeline::continue_read_data() {
    return http_response::status_response(this->output_stream_, 100, "Continue");
  }

  inline vds::async_task<vds::expected<void>> http_pipeline::finish_message() {
    co_return expected<void>();
  }  
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
