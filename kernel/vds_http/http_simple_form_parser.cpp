/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_simple_form_parser.h"

vds::async_task<vds::expected<void>> vds::http::simple_form_parser::on_field(const field_info & field)
{
  this->values_[field.name] = field.value;
  return expected<void>();
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::http::simple_form_parser::on_file(const file_info & file)
{
  return make_unexpected<std::runtime_error>("Unexpected file");
}
