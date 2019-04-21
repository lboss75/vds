/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "files_channel.h"
#include "db_model.h"
#include "user_message_transaction.h"
#include "user_manager.h"

namespace vds {
  namespace transactions {
    struct message_environment_t;
  }
}

vds::file_manager::file_in_storage::file_in_storage(const const_data_buffer& id, const filename& name,
  const std::string& mime_type, uint64_t size)
: id_(id), name_(name), mime_type_(mime_type), size_(size) {
}

vds::file_manager::files_channel::files_channel(const service_provider* sp,
  const std::shared_ptr<user_manager>& user_manager, const const_data_buffer& channel_id)
: sp_(sp), user_manager_(user_manager), channel_id_(channel_id){
}

vds::expected<std::optional<vds::file_manager::file_in_storage>> vds::file_manager::files_channel::looking_last_file(
  database_read_transaction & t,
  const filename name,
  const const_data_buffer file_hash) {

  auto result = vds::expected<std::optional<file_in_storage>>();

  CHECK_EXPECTED(this->user_manager_->walk_messages(
    this->channel_id_,
    t,
    [this, &result, name, file_hash](
      const transactions::user_message_transaction & message,
      const transactions::message_environment_t & /*message_environment*/) -> expected<bool> {
    for (const auto & file : message.files) {
      if (file.name == name.full_name() && (!file_hash || file_hash == file.file_id)) {
        result = vds::expected<std::optional<file_in_storage>>(file_in_storage(
          file.file_id,
          filename(file.name),
          file.mime_type,
          file.size));

        return false;
      }
    }
    return true;
  }));

  return result;
}

vds::async_task<vds::expected<vds::file_manager::file_operations::download_result_t>>
vds::file_manager::files_channel::download_file(
  const filename & file_name,
  const const_data_buffer & file_hash,
  const std::shared_ptr<stream_output_async<uint8_t>> & output_stream) {

  return this->sp_->get<file_manager::file_operations>()->download_file(
    this->user_manager_,
    this->channel_id_,
    file_name.full_name(),
    file_hash,
    output_stream);
}
