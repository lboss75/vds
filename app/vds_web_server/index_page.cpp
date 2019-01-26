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

vds::async_task<vds::expected<vds::http_message>> vds::index_page::create_channel(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager> & user_mng,
  const http_request & message) {

  CHECK_EXPECTED_ASYNC(co_await message.get_message().ignore_empty_body());

 
  auto type = message.get_parameter("type");
  auto name = message.get_parameter("name");

  co_return co_await api_controller::create_channel(
    user_mng,
    type,
    name);
}


vds::async_task<vds::expected<vds::http_message>> vds::index_page::create_message(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng,
  const http_request & request) {

  auto parser = std::make_shared<create_message_form>(sp ,user_mng);

  CHECK_EXPECTED_ASYNC(co_await parser->parse(request.get_message()));
  CHECK_EXPECTED_ASYNC(co_await parser->complete());
  
  co_return http_response::redirect("/");
}

class parse_request_form : public vds::http::form_parser<parse_request_form> {
  using base_class = vds::http::form_parser<parse_request_form>;
    
public:
  parse_request_form(const vds::service_provider * sp)
      : base_class(sp), successful_(false) {
  }

  vds::async_task<vds::expected<void>> on_field(const simple_field_info & /*field*/) {
    //Ignore return vds::make_unexpected<std::runtime_error>("Invalid field " + field.name);
    co_return vds::expected<void>();
  }

  vds::async_task<vds::expected<void>> on_file( const file_info & file) {
    GET_EXPECTED_ASYNC(buffer, co_await file.stream->read_all());
    GET_EXPECTED_VALUE_ASYNC(this->successful_, vds::user_manager::parse_join_request(
        buffer,
        this->userName_,
        this->userEmail_));
    co_return vds::expected<void>();
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

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::index_page::parse_join_request(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng,
  const http_request & message) {

  auto parser = std::make_shared<parse_request_form>(sp);

  CHECK_EXPECTED_ASYNC(co_await parser->parse(message.get_message()));

  auto result = std::make_shared<json_object>();
  if (parser->successful()) {
    result->add_property("successful", "true");
    result->add_property("name", parser->userName());
    result->add_property("email", parser->userEmail());
  }
  else {
    result->add_property("successful", "false");
  }

  co_return result;
}

class approve_join_request_form : public vds::http::form_parser<approve_join_request_form> {
  using base_class = vds::http::form_parser<approve_join_request_form>;

public:
  approve_join_request_form(
    const vds::service_provider * sp,
    const std::shared_ptr<vds::user_manager>& user_mng)
    : base_class(sp), user_mng_(user_mng), successful_(false) {

  }

  vds::async_task<vds::expected<void>> on_field(const simple_field_info & /*field*/) {
    co_return vds::expected<void>();
  }

  vds::async_task<vds::expected<void>> on_file( const file_info & file) {
    GET_EXPECTED_ASYNC(buffer, co_await file.stream->read_all());
    GET_EXPECTED_VALUE_ASYNC(this->successful_, co_await this->user_mng_->approve_join_request(buffer));
    co_return vds::expected<void>();
  }

  bool successful() const {
    return this->successful_;
  }

private:
  std::shared_ptr<vds::user_manager> user_mng_;
  bool successful_;
};


vds::async_task<vds::expected<vds::http_message>> vds::index_page::approve_join_request(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng,
  const http_request& message) {

  auto parser = std::make_shared<approve_join_request_form>(sp, user_mng);

  CHECK_EXPECTED_ASYNC(co_await parser->parse(message.get_message()));

  co_return http_response::redirect(parser->successful() ? "/?message=approve_successful" : "/?message=approve_failed");
}
