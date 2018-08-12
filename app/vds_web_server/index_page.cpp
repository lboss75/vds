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

vds::async_task<vds::http_message> vds::index_page::create_channel(const vds::service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng, const std::shared_ptr<_web_server>& web_server,
  const http_message& message) {

  auto parser = std::make_shared<http::simple_form_parser>();

  return parser->parse(sp, message).then([sp, user_mng, web_server, parser]() -> async_task<http_message> {
    auto name = parser->values().find("channelName");
    return api_controller::create_channel(sp, user_mng, name->second);
  });
}

class create_message_form : public vds::http::form_parser<create_message_form> {
public:
  create_message_form(
    const vds::service_provider & sp,
    const std::shared_ptr<vds::user_manager>& user_mng)
  : sp_(sp), user_mng_(user_mng) {
    
  }

  void on_field(const simple_field_info & field) {
    if(field.name == "channel_id") {
      this->channel_id_ = vds::base64::to_bytes(field.value);
    }
    else
    if (field.name == "message") {
      this->message_ = field.value;
    }
    else {
      throw std::runtime_error("Invalid field " + field.name);
    }
  }

  vds::async_task<> on_file(const file_info & file) {
    return this->sp_.get<vds::file_manager::file_operations>()->upload_file(
        this->sp_,
        this->user_mng_,
        file.file_name,
        file.mimetype,
        file.stream).then([pthis = this->shared_from_this(), file](const vds::transactions::user_message_transaction::file_info_t & file_info) {
      pthis->files_.push_back(file_info);
    });
  }

  vds::async_task<> complete() {
    return this->sp_.get<vds::file_manager::file_operations>()->create_message(
      this->sp_,
      this->user_mng_,
      this->channel_id_,
      this->message_,
      this->files_);
  }

private:
  vds::service_provider sp_;
  std::shared_ptr<vds::user_manager> user_mng_;
  vds::const_data_buffer channel_id_;
  std::list<vds::transactions::user_message_transaction::file_info_t> files_;
  std::string message_;
};

vds::async_task<vds::http_message> vds::index_page::create_message(const vds::service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng, const std::shared_ptr<_web_server>& web_server,
  const http_message& message) {

  auto parser = std::make_shared<create_message_form>(sp, user_mng);

  return parser->parse(sp, message).then([sp, user_mng, web_server, parser]() -> async_task<http_message> {
    return parser->complete().then([sp]() {
      return http_response::redirect(
        sp,
        "/");
    });
  });
}

class parse_request_form : public vds::http::form_parser<parse_request_form> {
public:
  parse_request_form(
      const vds::service_provider & sp)
      : sp_(sp), successful_(false) {

  }

  void on_field(const simple_field_info & field) {
    //Ignore throw std::runtime_error("Invalid field " + field.name);
  }

  vds::async_task<> on_file(const file_info & file) {

    return file.stream->read_all()
        .then([pthis = this->shared_from_this()](const vds::const_data_buffer & buffer){
         pthis->successful_ = vds::user_manager::parse_join_request(
        pthis->sp_,
        buffer,
         pthis->userName_,
         pthis->userEmail_);
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
  vds::service_provider sp_;
  bool successful_;
  std::string userName_;
  std::string userEmail_;
};

vds::async_task<vds::http_message> vds::index_page::parse_join_request(
    const vds::service_provider& sp,
    const std::shared_ptr<user_manager>& user_mng,
    const std::shared_ptr<_web_server>& web_server,
    const http_message& message) {

  auto parser = std::make_shared<parse_request_form>(sp);

  return parser->parse(sp, message)
      .then([sp, user_mng, web_server, parser]() -> async_task<http_message> {
    auto result = std::make_shared<json_object>();
        if(parser->successful()) {
          result->add_property("successful", "true");
          result->add_property("name", parser->userName());
          result->add_property("email", parser->userEmail());
        }
        else {
          result->add_property("successful", "false");
        }
    return vds::async_task<vds::http_message>::result(
        http_response::simple_text_response(
            sp,
            result->json_value::str(),
            "application/json; charset=utf-8"));
  });
}

class approve_join_request_form : public vds::http::form_parser<approve_join_request_form> {
public:
  approve_join_request_form(
    const vds::service_provider & sp,
    const std::shared_ptr<vds::user_manager>& user_mng)
    : sp_(sp), user_mng_(user_mng), successful_(false) {

  }

  void on_field(const simple_field_info & field) {
    //Ignore
  }

  vds::async_task<> on_file(const file_info & file) {
    auto pthis = this->shared_from_this();
    return file.stream->read_all()
      .then([pthis](const vds::const_data_buffer & buffer) {
      return pthis->user_mng_->approve_join_request(pthis->sp_, buffer);
    }).then([pthis](bool result) {
      pthis->successful_ = result;
    });
  }

  bool successful() const {
    return this->successful_;
  }

private:
  vds::service_provider sp_;
  std::shared_ptr<vds::user_manager> user_mng_;
  bool successful_;
};


vds::async_task<vds::http_message> vds::index_page::approve_join_request(const vds::service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng, const std::shared_ptr<_web_server>& web_server,
  const http_message& message) {

  auto parser = std::make_shared<approve_join_request_form>(sp, user_mng);

  return parser->parse(sp, message)
    .then([sp, user_mng, web_server, parser]() -> async_task<http_message> {

    return vds::async_task<vds::http_message>::result(
      http_response::redirect(sp, parser->successful() ? "/?message=approve_successful" : "/?message=approve_failed"));
  });
}
