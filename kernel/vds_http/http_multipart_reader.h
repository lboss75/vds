#ifndef __VDS_HTTP_HTTP_MULTIPART_READER_H_
#define __VDS_HTTP_HTTP_MULTIPART_READER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>
#include "http_message.h"
#include "http_parser.h"
#include "http_form_part_parser.h"
#include "vds_debug.h"

namespace vds {
  class http_multipart_reader : public stream_output_async<uint8_t> {
  public:
    http_multipart_reader(
      const service_provider * sp,
      const std::string & boundary,
      lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>, http_message> part_handler,
      lambda_holder_t<vds::async_task<vds::expected<void>>> final_handler);

  async_task<expected<void>> write_async(
    const uint8_t *data,
    size_t len) override;

  private:
    const service_provider * sp_;
    const std::string boundary_;
    const std::string final_boundary_;

    lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>, http_message> part_handler_;
    lambda_holder_t<vds::async_task<vds::expected<void>>> final_handler_;

    std::shared_ptr<vds::stream_output_async<uint8_t>> current_part_;

    uint8_t buffer_[1024];
    size_t readed_;

    async_task<expected<void>> push_data(size_t len);
  };
}

#endif // __VDS_HTTP_HTTP_MULTIPART_READER_H_
