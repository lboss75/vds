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

vds::async_task<vds::expected<void>> vds::file_manager::file_operations::prepare_to_stop() {
  return this->impl_->prepare_to_stop();
}

vds::async_task<vds::expected<vds::file_manager::file_operations::download_result_t>>
vds::file_manager::file_operations::download_file(
  const std::shared_ptr<vds::user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const std::string & file_name,
  const const_data_buffer & file_hash) {
  return this->impl_->download_file(user_mng, channel_id, file_name, file_hash);
}

vds::async_task<vds::expected<vds::file_manager::file_operations::prepare_download_result_t>>
vds::file_manager::file_operations::prepare_download_file(
  const std::shared_ptr<vds::user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const std::string& file_name,
  const const_data_buffer & target_file) {
  return this->impl_->prepare_download_file(user_mng, channel_id, file_name, target_file);
}

vds::expected<std::shared_ptr<vds::stream_input_async<uint8_t>>> vds::file_manager::file_operations::download_stream(
  std::list<transactions::user_message_transaction::file_block_t> file_blocks) {
  return this->impl_->download_stream(std::move(file_blocks));
}


vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>
vds::file_manager::file_operations::upload_file(
  const std::string & name,
  const std::string & mime_type,
  const const_data_buffer & file_hash,
  lambda_holder_t<
    async_task<expected<void>>,
    transactions::user_message_transaction::file_info_t> final_handler) {
  return this->impl_->upload_file(name, mime_type, file_hash, std::move(final_handler));
}

vds::async_task<vds::expected<void>> vds::file_manager::file_operations::create_message(
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::shared_ptr<json_value>& message,
  const std::list<transactions::user_message_transaction::file_info_t>& files) {
  return this->impl_->create_message(user_mng, channel_id, message, files);

}

//////////////////////////////////////////////////////////////////////////////////////////////
vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>
vds::file_manager_private::_file_operations::upload_file(
  const std::string & name,
  const std::string & mime_type,
  const const_data_buffer & file_hash,
  lambda_holder_t<
  async_task<expected<void>>,
  transactions::user_message_transaction::file_info_t> final_handler) {

  auto task = std::make_shared<_upload_stream_task>(
    this->sp_,
    [name, mime_type, h = std::move(final_handler)](
      const const_data_buffer & result_hash,
      uint64_t total_size,
      std::list<transactions::user_message_transaction::file_block_t> file_blocks) {
        return h(transactions::user_message_transaction::file_info_t{
          name,
          mime_type,
          total_size,
          result_hash,
          std::move(file_blocks) });
      });

  task->set_file_hash(file_hash);
  return task;
}

vds::async_task<vds::expected<vds::file_manager::file_operations::download_result_t>>
vds::file_manager_private::_file_operations::download_file(
  
  const std::shared_ptr<user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const std::string & file_name,
  const const_data_buffer & file_hash) {

  auto result = std::make_shared<file_manager::file_operations::download_result_t>();
  std::list<transactions::user_message_transaction::file_block_t> download_tasks;
  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction(
      [pthis = this->shared_from_this(), user_mng, channel_id, file_name, file_hash, result, &download_tasks](database_transaction &t) -> expected<void> {
    
    CHECK_EXPECTED(user_mng->update(t));
    auto channel = user_mng->get_channel(channel_id);

    CHECK_EXPECTED(user_mng->walk_messages(
            channel_id,
            t,
            [pthis, result, file_name, file_hash, &download_tasks](
              const transactions::user_message_transaction &message,
              const transactions::message_environment_t & /*message_environment*/) -> expected<bool> {
          for (const auto & file : message.files) {
            if (file.name == file_name && (!file_hash || file_hash == file.file_id)) {
              result->file_hash = file.file_id;
              result->name = file.name;
              result->mime_type = file.mime_type;
              result->size = file.size;

              download_tasks = std::move(file.file_blocks);
              return false;
            }
          }
              return true;
            }));

        return expected<void>();
  }));

  if (!result->mime_type.empty()) {
    GET_EXPECTED_VALUE_ASYNC(result->body, this->download_stream(std::move(download_tasks)));
  }

  co_return *result;
}

vds::async_task<vds::expected<vds::file_manager::file_operations::prepare_download_result_t>>
vds::file_manager_private::_file_operations::prepare_download_file(
  const std::shared_ptr<user_manager> & user_mng,
  const const_data_buffer & channel_id,
  const std::string& file_name,
  const const_data_buffer & target_file) {

  file_manager::file_operations::prepare_download_result_t result;
  std::list<std::function<async_task<expected<void>>()>> final_tasks;
  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_read_transaction(
    [pthis = this->shared_from_this(), user_mng, channel_id, file_name, target_file, &result, &final_tasks](database_read_transaction &t) ->expected<void> {
    auto channel = user_mng->get_channel(channel_id);

    CHECK_EXPECTED(user_mng->walk_messages(
      channel_id,
      t,
      [pthis, &t, &result, file_name, target_file, &final_tasks](
        const transactions::user_message_transaction &message,
        const transactions::message_environment_t & /*message_environment*/) -> expected<bool> {
      for (const auto & file : message.files) {
        if (file.name == file_name && (!target_file || target_file == file.file_id)) {
          result.file_hash = file.file_id;
          result.name = file.name;
          result.mime_type = file.mime_type;
          result.size = file.size;
          GET_EXPECTED_VALUE(result.blocks, pthis->prepare_download_stream(t, final_tasks, file.file_blocks));

          uint16_t ready_blocks = 0;
          for (auto & p : result.blocks) {
            for (auto & pr : p.second.replicas) {
              if (pr.second.size() >= dht::network::service::MIN_DISTRIBUTED_PIECES) {
                ++ready_blocks;
              }
            }
          }

          if(ready_blocks >= dht::network::service::MIN_HORCRUX) {
            result.progress = 100;
          }
          else {
            result.progress = 100 * ready_blocks / dht::network::service::MIN_HORCRUX;
            if (result.progress > 99) {
              result.progress = 99;
            }
          }
          return false;
        }
      }
      return true;
    }));

      return expected<void>();
    }));

  while(!final_tasks.empty()) {
    CHECK_EXPECTED_ASYNC(co_await final_tasks.front()());
    final_tasks.pop_front();
  }

  if (result.mime_type.empty()) {
    co_return vds::make_unexpected<vds_exceptions::not_found>();
  }
  co_return result;
}

vds::expected<std::shared_ptr<vds::stream_input_async<uint8_t>>>
vds::file_manager_private::_file_operations::download_stream(
  std::list<transactions::user_message_transaction::file_block_t> file_blocks)
{
  return std::make_shared<download_stream_t>(this->sp_, std::move(file_blocks));
}

vds::async_task<vds::expected<void>> vds::file_manager_private::_file_operations::create_message(
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::shared_ptr<json_value>& message,
  const std::list<transactions::user_message_transaction::file_info_t>& files) {
  return this->sp_->get<db_model>()->async_transaction(
    [pthis = this->shared_from_this(),
    user_mng,
    channel_id,
    message,
    files](database_transaction& t) -> expected<void> {

    auto channel = user_mng->get_channel(channel_id);
    if (!channel->write_public_key()) {
      pthis->sp_->get<logger>()->error(
        ThisModule,
        "Channel %s don't have write key",
        base64::from_bytes(channel_id).c_str());
      return vds::make_unexpected<vds_exceptions::access_denied_error>("User don't have write permission");
    }

    GET_EXPECTED(log, transactions::transaction_block_builder::create(pthis->sp_, t));
    CHECK_EXPECTED(channel->add_log(
      log,
      message_create<transactions::user_message_transaction>(
        message,
        files)));

    CHECK_EXPECTED(pthis->sp_->get<dht::network::client>()->save(
      pthis->sp_,
      log,
      t));

    return expected<void>();
  });
}

void vds::file_manager_private::_file_operations::start(const service_provider* sp) {
  this->sp_ = sp;
}

void vds::file_manager_private::_file_operations::stop() {
}

vds::async_task<vds::expected<void>> vds::file_manager_private::_file_operations::prepare_to_stop() {
  co_return expected<void>();
}

vds::expected<std::map<vds::const_data_buffer, vds::dht::network::client::block_info_t>>
vds::file_manager_private::_file_operations::prepare_download_stream(
  database_read_transaction & t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const std::list<vds::transactions::user_message_transaction::file_block_t> &file_blocks_param) {

  auto result = std::make_shared<std::map<vds::const_data_buffer, vds::dht::network::client::block_info_t>>();
  std::list<vds::transactions::user_message_transaction::file_block_t> file_blocks = file_blocks_param;

  while (!file_blocks.empty()) {
    auto network_client = this->sp_->get<dht::network::client>();

    vds::dht::network::client::block_info_t info;
    GET_EXPECTED_VALUE(info, network_client->prepare_restore(t, final_tasks, dht::network::client::chunk_info{
        file_blocks.begin()->block_id,
        file_blocks.begin()->block_key,
        file_blocks.begin()->replica_hashes }));

    (*result)[file_blocks.begin()->block_id] = info;

    file_blocks.pop_front();
  }

  return std::move(*result);
}

vds::file_manager_private::_file_operations::download_stream_t::download_stream_t(
  const service_provider * sp,
  std::list<transactions::user_message_transaction::file_block_t> file_blocks)
  : sp_(sp), file_blocks_(std::move(file_blocks)), readed_(0) {
}

vds::async_task<vds::expected<size_t>> vds::file_manager_private::_file_operations::download_stream_t::read_async(
  uint8_t * buffer,
  size_t len) {
  for (;;) {
    if (this->buffer_.size() > this->readed_) {
      auto size = this->buffer_.size() - this->readed_;
      if (size > len) {
        size = len;
      }

      memcpy(buffer, this->buffer_.data() + this->readed_, size);
      this->readed_ += size;
      co_return size;
    }

    if(!this->file_blocks_.empty()) {
      auto network_client = this->sp_->get<dht::network::client>();
      GET_EXPECTED_VALUE_ASYNC(this->buffer_, co_await network_client->restore(dht::network::client::chunk_info{
          this->file_blocks_.begin()->block_id,
          this->file_blocks_.begin()->block_key,
          this->file_blocks_.begin()->replica_hashes }));

      vds_assert(this->buffer_.size() == this->file_blocks_.begin()->block_size);
      this->readed_ = 0;

      this->file_blocks_.pop_front();
    }
    else {
      co_return 0;
    }
  }
}
