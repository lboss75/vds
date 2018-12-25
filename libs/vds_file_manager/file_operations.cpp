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
#include "file_manager_service.h"

vds::file_manager::file_operations::file_operations()
  : impl_(new file_manager_private::_file_operations()) {
}

void vds::file_manager::file_operations::start(const service_provider* sp) {
  this->impl_->start(sp);
}

void vds::file_manager::file_operations::stop() {
  this->impl_->stop();
}

vds::async_task<void> vds::file_manager::file_operations::prepare_to_stop() {
  return this->impl_->prepare_to_stop();
}

vds::async_task<vds::file_manager::file_operations::download_result_t>
vds::file_manager::file_operations::download_file(
  const std::shared_ptr<vds::user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const const_data_buffer & target_file,
  const std::shared_ptr<stream_output_async<uint8_t>> & output_stream) {
  return this->impl_->download_file(user_mng, channel_id, target_file, output_stream);
}

vds::async_task<vds::file_manager::file_operations::prepare_download_result_t>
vds::file_manager::file_operations::prepare_download_file(
  const std::shared_ptr<vds::user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const const_data_buffer & target_file) {
  return this->impl_->prepare_download_file(user_mng, channel_id, target_file);
}


vds::async_task<vds::transactions::user_message_transaction::file_info_t>
vds::file_manager::file_operations::upload_file(
  
  const std::shared_ptr<user_manager>& user_mng,
  const std::string & name,
  const std::string & mime_type,
  const std::shared_ptr<stream_input_async<uint8_t>>& input_stream) {
  return this->impl_->upload_file(user_mng, name, mime_type, input_stream);
}

vds::async_task<void> vds::file_manager::file_operations::create_message(
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::shared_ptr<json_value>& message,
  const std::list<transactions::user_message_transaction::file_info_t>& files) {
  return this->impl_->create_message(user_mng, channel_id, message, files);

}

//////////////////////////////////////////////////////////////////////////////////////////////
vds::async_task<vds::transactions::user_message_transaction::file_info_t> vds::file_manager_private::_file_operations::upload_file(
  
  const std::shared_ptr<user_manager>& user_mng,
  const std::string & name,
  const std::string & mime_type,
  const std::shared_ptr<stream_input_async<uint8_t>>& input_stream) {

  auto file_info = co_await this->pack_file(input_stream);

  co_return transactions::user_message_transaction::file_info_t{
        name,
        mime_type,
        file_info.total_size,
        file_info.total_hash,
        file_info.file_blocks };
}

vds::async_task<vds::file_manager::file_operations::download_result_t> vds::file_manager_private::_file_operations::download_file(
  
  const std::shared_ptr<user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const const_data_buffer & target_file,
  const std::shared_ptr<stream_output_async<uint8_t>> & output_stream) {

  auto result = std::make_shared<file_manager::file_operations::download_result_t>();
  co_await this->sp_->get<db_model>()->async_transaction(
      [pthis = this->shared_from_this(), user_mng, channel_id, target_file, output_stream, result](database_transaction &t) -> bool {
        auto channel = user_mng->get_channel(channel_id);

        user_mng->walk_messages(
            channel_id,
            t,
            [pthis, result, target_file, output_stream](
              const transactions::user_message_transaction &message,
              const transactions::message_environment_t & /*message_environment*/) -> bool {
          for (const auto & file : message.files) {
            if (target_file == file.file_id) {
              result->name = file.name;
              result->mime_type = file.mime_type;
              result->size = file.size;
              mt_service::async(pthis->sp_, [pthis, output_stream, fb = file.file_blocks]() {
                try {
                  pthis->download_stream(output_stream, fb).get();
                }
                catch(...){}
              });
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

vds::async_task<vds::file_manager::file_operations::prepare_download_result_t> vds::file_manager_private::_file_operations::prepare_download_file(
  const std::shared_ptr<user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const const_data_buffer & target_file) {

  auto result = std::make_shared<file_manager::file_operations::prepare_download_result_t>();
  co_await this->sp_->get<db_model>()->async_read_transaction(
    [pthis = this->shared_from_this(), user_mng, channel_id, target_file, result](database_read_transaction &t) -> bool {
    auto channel = user_mng->get_channel(channel_id);

    user_mng->walk_messages(
      channel_id,
      t,
      [pthis, &t, result, target_file](
        const transactions::user_message_transaction &message,
        const transactions::message_environment_t & /*message_environment*/) -> bool {
      for (const auto & file : message.files) {
        if (target_file == file.file_id) {
          result->name = file.name;
          result->mime_type = file.mime_type;
          result->size = file.size;
          result->blocks = pthis->prepare_download_stream(t, file.file_blocks).get();

          uint16_t ready_blocks = 0;
          for (auto & p : result->blocks) {
            for (auto & pr : p.second.replicas) {
              if (pr.second.size() >= dht::network::service::MIN_DISTRIBUTED_PIECES) {
                ++ready_blocks;
              }
            }
          }

          if(ready_blocks >= dht::network::service::MIN_HORCRUX) {
            result->progress = 100;
          }
          else {
            result->progress = 100 * ready_blocks / dht::network::service::MIN_HORCRUX;
            if (result->progress > 99) {
              result->progress = 99;
            }
          }
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

vds::async_task<void> vds::file_manager_private::_file_operations::create_message(
  
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::shared_ptr<json_value>& message,
  const std::list<transactions::user_message_transaction::file_info_t>& files) {
  return this->sp_->get<db_model>()->async_transaction(
    [pthis = this->shared_from_this(),
    user_mng,
    channel_id,
    message,
    files](database_transaction& t) {

    auto channel = user_mng->get_channel(channel_id);
    if (!channel->write_cert()) {
      pthis->sp_->get<logger>()->error(
        ThisModule,
        "Channel %s don't have write cert",
        base64::from_bytes(channel_id).c_str());
      throw vds_exceptions::access_denied_error("User don't have write permission");
    }

    transactions::transaction_block_builder log(pthis->sp_, t);
    channel->add_log(
      log,
      message_create<transactions::user_message_transaction>(
        message,
        files));

    log.save(
      pthis->sp_,
      t,
      user_mng->get_current_user().user_certificate(),
      user_mng->get_current_user_private_key());

    return true;
  });
}

void vds::file_manager_private::_file_operations::start(const service_provider* sp) {
  this->sp_ = sp;
}

void vds::file_manager_private::_file_operations::stop() {
}

vds::async_task<void> vds::file_manager_private::_file_operations::prepare_to_stop() {
  co_return;
}

struct buffer_data : public std::enable_shared_from_this<buffer_data> {
  uint8_t buffer[vds::dht::network::service::BLOCK_SIZE];
};

vds::async_task<vds::file_manager_private::_file_operations::pack_file_result>
vds::file_manager_private::_file_operations::pack_file(
  
  const std::shared_ptr<stream_input_async<uint8_t>>& input_stream) const {
  auto task = std::make_shared<_upload_stream_task>();
  auto file_blocks = co_await task->start(this->sp_, input_stream);
  
  co_return pack_file_result { task->result_hash(), task->total_size(), file_blocks };
}

vds::async_task<void> vds::file_manager_private::_file_operations::download_stream(
  const std::shared_ptr<vds::stream_output_async<uint8_t>> & target_stream_param,
  const std::list<vds::transactions::user_message_transaction::file_block_t> &file_blocks_param) {

  std::shared_ptr<vds::stream_output_async<uint8_t>> target_stream = target_stream_param;
  std::list<vds::transactions::user_message_transaction::file_block_t> file_blocks = file_blocks_param;

  while (!file_blocks.empty()) {
    auto network_client = this->sp_->get<dht::network::client>();

    const_data_buffer data = co_await network_client->restore(dht::network::client::chunk_info{
        file_blocks.begin()->block_id,
        file_blocks.begin()->block_key,
        file_blocks.begin()->replica_hashes });

    vds_assert(data.size() == file_blocks.begin()->block_size);
    auto buffer = std::make_shared<const_data_buffer>(data);
    co_await target_stream->write_async(buffer->data(), buffer->size());

    file_blocks.pop_front();
  }

  co_await target_stream->write_async(nullptr, 0);
}

vds::async_task<std::map<vds::const_data_buffer, vds::dht::network::client::block_info_t>>
vds::file_manager_private::_file_operations::prepare_download_stream(
  database_read_transaction & t,
  const std::list<vds::transactions::user_message_transaction::file_block_t> &file_blocks_param) {

  auto result = std::make_shared<std::map<vds::const_data_buffer, vds::dht::network::client::block_info_t>>();
  std::list<vds::transactions::user_message_transaction::file_block_t> file_blocks = file_blocks_param;

  while (!file_blocks.empty()) {
    auto network_client = this->sp_->get<dht::network::client>();

    auto info = co_await network_client->prepare_restore(t, dht::network::client::chunk_info{
        file_blocks.begin()->block_id,
        file_blocks.begin()->block_key,
        file_blocks.begin()->replica_hashes });

    (*result)[file_blocks.begin()->block_id] = info;

    file_blocks.pop_front();
  }

  co_return std::move(*result);
}
