/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_multipart_reader.h"

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>
vds::http_multipart_reader::parse(
  const service_provider * sp,
  const std::string & boundary,
  lambda_holder_t<
    vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>,
    http_message&&> part_handler,
  lambda_holder_t<
    vds::async_task<vds::expected<void>>> final_handler)
{
  return vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>();
}

vds::async_task<vds::expected<void>> vds::http_multipart_reader::write_async(const uint8_t* data, size_t len) {

}
