/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <dht_network_client.h>
#include "stdafx.h"
#include "file_operations.h"
#include "file.h"
#include "db_model.h"
#include "private/file_operations_p.h"
#include "service_provider.h"
#include "transaction_block_builder.h"
#include "user_message_transaction.h"
#include "user_manager.h"
#include "vds_exceptions.h"
#include "chunk.h"
#include "logger.h"
#include "encoding.h"
#include "private/upload_stream_task_p.h"
#include "member_user.h"

vds::file_manager::file_operations::file_operations()
  : impl_(new file_manager_private::_file_operations()) {
}

std::future<vds::file_manager::file_operations::download_result_t>
vds::file_manager::file_operations::download_file(
  const service_provider& sp,
  const std::shared_ptr<user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const const_data_buffer & target_file,
  const std::shared_ptr<stream_output_async<uint8_t>> & output_stream) {
  return this->impl_->download_file(sp, user_mng, channel_id, target_file, output_stream);
}

std::future<vds::transactions::user_message_transaction::file_info_t>
vds::file_manager::file_operations::upload_file(
  const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::string & name,
  const std::string & mime_type,
  const std::shared_ptr<stream_input_async<uint8_t>>& input_stream) {
  return this->impl_->upload_file(sp, user_mng, name, mime_type, input_stream);
}

std::future<void> vds::file_manager::file_operations::create_message(const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng, const const_data_buffer& channel_id, const std::string& message,
  const std::list<transactions::user_message_transaction::file_info_t>& files) {
  return this->impl_->create_message(sp, user_mng, channel_id, message, files);

}

//////////////////////////////////////////////////////////////////////////////////////////////
std::future<vds::transactions::user_message_transaction::file_info_t> vds::file_manager_private::_file_operations::upload_file(
  const service_provider& paren_sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::string & name,
  const std::string & mime_type,
  const std::shared_ptr<stream_input_async<uint8_t>>& input_stream) {

  auto sp = paren_sp.create_scope(__FUNCTION__);
  auto file_info = co_await this->pack_file(sp, input_stream);

  co_return transactions::user_message_transaction::file_info_t{
        name,
        mime_type,
        file_info.total_size,
        file_info.total_hash,
        file_info.file_blocks };
}

std::future<vds::file_manager::file_operations::download_result_t> vds::file_manager_private::_file_operations::download_file(
  const service_provider& parent_sp,
  const std::shared_ptr<user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const const_data_buffer & target_file,
  const std::shared_ptr<stream_output_async<uint8_t>> & output_stream) {
  auto result = std::make_shared<file_manager::file_operations::download_result_t>();
  auto sp = parent_sp.create_scope(__FUNCTION__);
  co_await sp.get<db_model>()->async_transaction(
      sp,
      [pthis = this->shared_from_this(), sp, user_mng, channel_id, target_file, output_stream, result](database_transaction &t) -> bool {
        auto channel = user_mng->get_channel(sp, channel_id);

        user_mng->walk_messages(
            sp,
            channel_id,
            t,
            [sp, pthis, result, target_file, output_stream](const transactions::user_message_transaction &message) -> bool {
          for (const auto & file : message.files) {
            if (target_file == file.file_id) {
              result->name = file.name;
              result->mime_type = file.mime_type;
              result->size = file.size;
              pthis->download_stream(sp, output_stream, file.file_blocks);
              return false;
            }
          }
              return true;
            });

        return true;
  });

  if (result->mime_type.empty()) {
    throw vds_exceptions::not_found();
  }
  co_return *result;
}

std::future<void> vds::file_manager_private::_file_operations::create_message(const service_provider& parent_sp,
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::string& message,
  const std::list<transactions::user_message_transaction::file_info_t>& files) {
  auto sp = parent_sp.create_scope(__FUNCTION__);
  return sp.get<db_model>()->async_transaction(
    sp,
    [pthis = this->shared_from_this(),
    sp,
    user_mng,
    channel_id,
    message,
    files](database_transaction& t) {

    auto channel = user_mng->get_channel(sp, channel_id);
    if (!channel->write_cert()) {
      sp.get<logger>()->error(
        ThisModule,
        sp,
        "Channel %s don't have write cert",
        base64::from_bytes(channel_id).c_str());
      throw vds_exceptions::access_denied_error("User don't have write permission");
    }

    transactions::transaction_block_builder log(sp, t);
    channel->add_log(
      log,
      message_create<transactions::user_message_transaction>(
        message,
        files,
        std::string()));

    log.save(
      sp,
      t,
      user_mng->get_current_user().user_certificate(),
      user_mng->get_current_user_private_key());

    return true;
  });
}

struct buffer_data : public std::enable_shared_from_this<buffer_data> {
  uint8_t buffer[vds::dht::network::service::BLOCK_SIZE];
};

std::future<vds::file_manager_private::_file_operations::pack_file_result>
vds::file_manager_private::_file_operations::pack_file(
  const service_provider& sp,
  const std::shared_ptr<stream_input_async<uint8_t>>& input_stream) const {
  auto task = std::make_shared<_upload_stream_task>();
  auto file_blocks = co_await task->start(sp, input_stream);
  
  co_return pack_file_result{ task->result_hash(), task->total_size(), file_blocks };
}

std::future<void> vds::file_manager_private::_file_operations::download_stream(
  const vds::service_provider &sp,
  const std::shared_ptr<vds::stream_output_async<uint8_t>> &target_stream,
  const std::list<vds::transactions::user_message_transaction::file_block_t> &file_blocks) {
  while (!file_blocks.empty()) {
    auto network_client = sp.get<dht::network::client>();

    const_data_buffer data = co_await network_client->restore(sp, dht::network::client::chunk_info{
        file_blocks.begin()->block_id,
        file_blocks.begin()->block_key,
        file_blocks.begin()->replica_hashes });

    vds_assert(data.size() == file_blocks.begin()->block_size);
    auto buffer = std::make_shared<const_data_buffer>(data);
    co_await target_stream->write_async(sp, buffer->data(), buffer->size());

    auto f = file_blocks;
    f.pop_front();
  }

  co_await target_stream->write_async(sp, nullptr, 0);
}
