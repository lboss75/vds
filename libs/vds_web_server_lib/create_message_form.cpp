/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/create_message_form.h"

vds::create_message_form::create_message_form(const vds::service_provider* sp,
                                              const std::shared_ptr<vds::user_manager>& user_mng): base_class(sp),
                                                                                                   user_mng_(user_mng),
                                                                                                   message_(
                                                                                                     new vds::
                                                                                                     json_object()) {
  this->message_->add_property("$type", "SimpleMessage");
}

vds::async_task<vds::expected<void>> vds::create_message_form::on_field(const simple_field_info& field) {
  if (field.name == "channel_id") {
    GET_EXPECTED_VALUE_ASYNC(this->channel_id_, vds::base64::to_bytes(field.value));
  }
  else if (field.name == "message") {
    this->message_->add_property("message", field.value);
  }
  else {
    co_return vds::make_unexpected<std::runtime_error>("Invalid field " + field.name);
  }
  co_return expected<void>();
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::create_message_form::on_file(
  const file_info& file) {
  const_data_buffer file_hash;
  for (auto& h : file.headers) {
    static char vds_sha256[] = "X-VDS-SHA256:";
    if (0 == strncmp(h.c_str(), vds_sha256, sizeof(vds_sha256) - 1)) {
      GET_EXPECTED_VALUE(file_hash, base64::to_bytes(h.c_str() + (sizeof(vds_sha256) - 1)));
      break;
    }
  }
    
  return this->sp_->get<vds::file_manager::file_operations>()->upload_file(
    this->user_mng_,
    file.file_name,
    file.mimetype,
    file_hash,
    [pthis = this->shared_from_this()](vds::transactions::user_message_transaction::file_info_t && file_info) -> async_task<expected<void>> {
    static_cast<create_message_form *>(pthis.get())->files_.push_back(file_info);
    return expected<void>();
  });
}

vds::async_task<vds::expected<void>> vds::create_message_form::complete() {
  if (!this->channel_id_) {
    return make_unexpected<std::runtime_error>("Specify channel_id");
  }
  return this->sp_->get<vds::file_manager::file_operations>()->create_message(
    this->user_mng_,
    this->channel_id_,
    this->message_,
    this->files_);
}
