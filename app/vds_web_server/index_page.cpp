/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <user_manager.h>
#include "stdafx.h"
#include "private/index_page.h"
#include "http_simple_form_parser.h"
#include "private/api_controller.h"
#include "user_channel.h"
#include "http_form_parser.h"
#include "file_operations.h"
#include "http_response.h"
#include "http_request.h"

vds::async_task<vds::http_message> vds::index_page::create_channel(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager> & user_mng,
  const http_request & message) {

  auto parser = std::make_shared<http::simple_form_parser>(sp);

  co_await parser->parse(message.get_message());
  
  auto name = parser->values().find("channelName");
  co_return co_await api_controller::create_channel(user_mng, name->second);
}

class create_message_form : public vds::http::form_parser<create_message_form> {
  using base_class = vds::http::form_parser<create_message_form>;
public:
  create_message_form(
    const vds::service_provider * sp,
    const std::shared_ptr<vds::user_manager>& user_mng)
  : base_class(sp), user_mng_(user_mng), message_(new vds::json_object()) {
    this->message_->add_property("$type", "SimpleMessage");
  }

  void on_field(const simple_field_info & field) {
    if(field.name == "channel_id") {
      this->channel_id_ = vds::base64::to_bytes(field.value);
    }
    else
    if (field.name == "message") {
      this->message_->add_property("message", field.value);
    }
    else {
      throw std::runtime_error("Invalid field " + field.name);
    }
  }

  vds::async_task<void> on_file( const file_info & file) {
    auto file_info = co_await this->sp_->get<vds::file_manager::file_operations>()->upload_file(
      this->user_mng_,
      file.file_name,
      file.mimetype,
      file.stream);
    
    this->files_.push_back(file_info);
  }

  vds::async_task<void> complete() {
    return this->sp_->get<vds::file_manager::file_operations>()->create_message(
      this->user_mng_,
      this->channel_id_,
      this->message_,
      this->files_);
  }

private:
  std::shared_ptr<vds::user_manager> user_mng_;
  vds::const_data_buffer channel_id_;
  std::list<vds::transactions::user_message_transaction::file_info_t> files_;
  std::shared_ptr<vds::json_object> message_;
};

vds::async_task<vds::http_message> vds::index_page::create_message(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng,
  const http_request & request) {

  auto parser = std::make_shared<create_message_form>(sp ,user_mng);

  co_await parser->parse(request.get_message());
  co_await parser->complete();
  
  co_return http_response::redirect("/");
}

class parse_request_form : public vds::http::form_parser<parse_request_form> {
  using base_class = vds::http::form_parser<parse_request_form>;
    
public:
  parse_request_form(const vds::service_provider * sp)
      : base_class(sp), successful_(false) {
  }

  void on_field(const simple_field_info & field) {
    //Ignore throw std::runtime_error("Invalid field " + field.name);
  }

  vds::async_task<void> on_file( const file_info & file) {

    auto buffer = co_await file.stream->read_all();

    this->successful_ = vds::user_manager::parse_join_request(
        buffer,
        this->userName_,
        this->userEmail_);
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

vds::async_task<std::shared_ptr<vds::json_value>> vds::index_page::parse_join_request(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng,
  const http_request & message) {

  auto parser = std::make_shared<parse_request_form>(sp);

  co_await parser->parse(message.get_message());

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

  void on_field(const simple_field_info & field) {
    //Ignore
  }

  vds::async_task<void> on_file( const file_info & file) {
    auto buffer = co_await file.stream->read_all();

    this->successful_ = co_await this->user_mng_->approve_join_request(buffer);
  }

  bool successful() const {
    return this->successful_;
  }

private:
  std::shared_ptr<vds::user_manager> user_mng_;
  bool successful_;
};


vds::async_task<vds::http_message> vds::index_page::approve_join_request(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng,
  const http_request& message) {

  auto parser = std::make_shared<approve_join_request_form>(sp, user_mng);

  co_await parser->parse(message.get_message());

  co_return http_response::redirect(parser->successful() ? "/?message=approve_successful" : "/?message=approve_failed");
}
