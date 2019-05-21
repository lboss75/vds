/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "user_manager.h"
#include "private/index_page.h"
#include "http_simple_form_parser.h"
#include "private/api_controller.h"
#include "user_channel.h"
#include "http_form_parser.h"
#include "file_operations.h"
#include "http_response.h"
#include "http_request.h"
#include "private/create_message_form.h"

vds::async_task<vds::expected<void>> vds::index_page::create_channel(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager> & user_mng,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_message & message) {

  const auto type = message.get_parameter("type");
  const auto name = message.get_parameter("name");

  co_return co_await api_controller::create_channel(
    user_mng,
    output_stream,
    type,
    name);
}


vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::index_page::create_message(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_message & request) {

  auto parser = std::make_shared<create_message_form>(sp, user_mng);

  return parser->parse(request, [parser, output_stream]()->vds::async_task<vds::expected<void>> {
    CHECK_EXPECTED_ASYNC(co_await parser->complete());
    co_return co_await http_response::redirect(output_stream, "/");
  });
}

class parse_request_form : public vds::http::form_parser {
  using base_class = vds::http::form_parser;
    
public:
  parse_request_form(const vds::service_provider * sp)
      : base_class(sp), successful_(false) {
  }

  vds::async_task<vds::expected<void>> on_field(const field_info & /*field*/) override {
    //Ignore return vds::make_unexpected<std::runtime_error>("Invalid field " + field.name);
    co_return vds::expected<void>();
  }

  vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> on_file(
    const file_info & file) override {

    co_return std::make_shared<vds::collect_data>([pthis_ = this->shared_from_this()](vds::const_data_buffer && body)-> vds::async_task<vds::expected<void>> {
      auto pthis = static_cast<parse_request_form *>(pthis_.get());
      GET_EXPECTED_VALUE(pthis->successful_, vds::user_manager::parse_join_request(
        std::move(body),
        pthis->userName_,
        pthis->userEmail_));
      return vds::expected<void>();
    });

  }

  bool successful() const {
    return this->successful_;
  }

  const std::string &userName() const {
    return this->userName_;
  }

  const std::string & userEmail() const {
    return this->userEmail_;
  }

private:
  bool successful_;
  std::string userName_;
  std::string userEmail_;
};

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::index_page::parse_join_request(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& /*user_mng*/,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_message & message) {

  auto parser = std::make_shared<parse_request_form>(sp);

  return parser->parse(message, [parser, output_stream]()->vds::async_task<vds::expected<void>> {

    auto result = std::make_shared<json_object>();
    if (parser->successful()) {
      result->add_property("successful", "true");
      result->add_property("name", parser->userName());
      result->add_property("email", parser->userEmail());
    }
    else {
      result->add_property("successful", "false");
    }

    GET_EXPECTED_ASYNC(str, result->json_value::str());

    co_return co_await http_response::simple_text_response(
      output_stream,
      str,
      "application/json; charset=utf-8");
  });
}

class approve_join_request_form : public vds::http::form_parser {
  using base_class = vds::http::form_parser;

public:
  approve_join_request_form(
    const vds::service_provider * sp,
    const std::shared_ptr<vds::user_manager>& user_mng)
    : base_class(sp), user_mng_(user_mng), successful_(false) {

  }

  vds::async_task<vds::expected<void>> on_field(const field_info & /*field*/) override {
    co_return vds::expected<void>();
  }

  vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> on_file(
    const file_info & file) override {

    co_return std::make_shared<vds::collect_data>(
      [pthis_ = this->shared_from_this()](vds::const_data_buffer && buffer)->vds::async_task<vds::expected<void>> {
      auto pthis = static_cast<approve_join_request_form *>(pthis_.get());
      GET_EXPECTED_VALUE_ASYNC(pthis->successful_, co_await pthis->user_mng_->approve_join_request(buffer));
      co_return vds::expected<void>();
    });
  }

  bool successful() const {
    return this->successful_;
  }

private:
  std::shared_ptr<vds::user_manager> user_mng_;
  bool successful_;
};


vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::index_page::approve_join_request(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_message & message) {

  auto parser = std::make_shared<approve_join_request_form>(sp, user_mng);

  return parser->parse(message, [parser, output_stream, message]()->vds::async_task<vds::expected<void>> {
    return http_response::redirect(
      output_stream,
      parser->successful() ? "/?message=approve_successful" : "/?message=approve_failed");
  });
}
