#include "stdafx.h"
#include "private/upload_stream_task_p.h"
#include "db_model.h"
#include "dht_network_client.h"
#include "string_format.h"
#include "logger.h"

vds::_upload_stream_task::_upload_stream_task(
  const service_provider * sp,
  lambda_holder_t<
  async_task<expected<void>>,
  const const_data_buffer & /*result_hash*/,
  uint64_t /*total_size*/,
  std::list<transactions::user_message_transaction::file_block_t>> result_handler)
: sp_(sp), total_size_(0), readed_(0), result_handler_(std::move(result_handler)){
}

vds::async_task<vds::expected<void>> vds::_upload_stream_task::write_async(const uint8_t* data, size_t len) {

  if (0 == this->total_size_) {
    GET_EXPECTED_VALUE_ASYNC(this->total_hash_, hash::create(hash::sha256()));
  }

  auto network_client = this->sp_->get<dht::network::client>();

  if(0 == len) {
    if (this->current_target_) {
      CHECK_EXPECTED_ASYNC(co_await this->current_target_->write_async(nullptr, 0));
      auto block_info = co_await network_client->finish_save(this->sp_, this->current_target_);

      this->current_target_.reset();

      this->result_.push_back(transactions::user_message_transaction::file_block_t{
        /*block_id =*/ block_info.value().id,
        /*block_key =*/ block_info.value().key,
        /*object_ids*/block_info.value().object_ids,
        /*block_size =*/ this->readed_
       });
    }

    CHECK_EXPECTED_ASYNC(this->total_hash_.final());

    if (this->target_file_hash_.size() > 0 && this->target_file_hash_ != this->total_hash_.signature()) {
      co_return make_unexpected<std::runtime_error>(
        string_format("File hash changed during upload process: original hash: %s, current hash: %s",
          base64::from_bytes(this->target_file_hash_).c_str(),
          base64::from_bytes(this->total_hash_.signature()).c_str()));
    }

    CHECK_EXPECTED_ASYNC(co_await this->result_handler_(
      this->total_hash_.signature(),
      this->total_size_,
      std::move(this->result_)));
    co_return expected<void>();
  }
  else {
    this->sp_->get<logger>()->trace("HASH", "[%s]", std::string((const char *)data, len).c_str());
    CHECK_EXPECTED_ASYNC(this->total_hash_.update(data, len));
    this->total_size_ += len;
  }
  while (0 < len) {
    if (!this->current_target_) {
      GET_EXPECTED_VALUE_ASYNC(this->current_target_, network_client->start_save(this->sp_));
    }

    auto to_write = len;
    if (to_write + this->readed_ > dht::network::service::BLOCK_SIZE) {
      to_write = dht::network::service::BLOCK_SIZE - this->readed_;
    }
    CHECK_EXPECTED_ASYNC(co_await this->current_target_->write_async(data, to_write));
    this->readed_ += to_write;
    data += to_write;
    len -= to_write;

    if (dht::network::service::BLOCK_SIZE == this->readed_) {
      CHECK_EXPECTED_ASYNC(co_await this->current_target_->write_async(nullptr, 0));
      auto block_info = co_await network_client->finish_save(this->sp_, this->current_target_);
      this->current_target_.reset();

      this->result_.push_back(transactions::user_message_transaction::file_block_t{
        /*block_id =*/ block_info.value().id,
        /*block_key =*/ block_info.value().key,
        /*object_ids*/block_info.value().object_ids,
        /*block_size =*/ this->readed_
        });
      this->readed_ = 0;
    }
  }

  co_return expected<void>();
}
