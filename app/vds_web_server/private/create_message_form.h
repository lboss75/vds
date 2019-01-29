#ifndef __VDS_WEB_SERVER_CREATE_MESSAGE_FORM_H__
#define __VDS_WEB_SERVER_CREATE_MESSAGE_FORM_H__

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_form_parser.h"
#include "user_manager.h"
#include "file_operations.h"

namespace vds {
  class create_message_form : public http::form_parser {
    using base_class = vds::http::form_parser;
  public:
    create_message_form(
      const vds::service_provider * sp,
      const std::shared_ptr<vds::user_manager>& user_mng)
      : base_class(sp), user_mng_(user_mng), message_(new vds::json_object()) {
      this->message_->add_property("$type", "SimpleMessage");
    }

    vds::async_task<vds::expected<void>> on_field(const simple_field_info & field) override {
      if (field.name == "channel_id") {
        GET_EXPECTED_VALUE_ASYNC(this->channel_id_, vds::base64::to_bytes(field.value));
      }
      else
        if (field.name == "message") {
          this->message_->add_property("message", field.value);
        }
        else {
          co_return vds::make_unexpected<std::runtime_error>("Invalid field " + field.name);
        }
      co_return expected<void>();
    }

    vds::async_task<vds::expected<void>> on_file(const file_info & file) override {
        vds::transactions::user_message_transaction::file_info_t file_info;
      GET_EXPECTED_VALUE_ASYNC(file_info, co_await this->sp_->get<vds::file_manager::file_operations>()->upload_file(
        this->user_mng_,
        file.file_name,
        file.mimetype,
        file.stream));

      this->files_.push_back(file_info);
      co_return expected<void>();
    }

    vds::async_task<vds::expected<void>> complete() {
      if(!this->channel_id_) {
        return make_unexpected<std::runtime_error>("Specify channel_id");
      }
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
}

#endif//__VDS_WEB_SERVER_CREATE_MESSAGE_FORM_H__