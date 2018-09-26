
#include "stdafx.h"
#include "private/upload_stream_task_p.h"
#include "db_model.h"
#include "dht_network_client.h"

vds::_upload_stream_task::_upload_stream_task()
: total_hash_(hash::sha256()), total_size_(0), readed_(0) {
}

std::future<std::list<vds::transactions::user_message_transaction::file_block_t>> vds::_upload_stream_task::start(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream) {

  auto network_client = sp.get<dht::network::client>();
  co_await this->continue_read(sp, network_client, input_stream);
  co_return this->file_blocks_;
}

std::future<void>
vds::_upload_stream_task::continue_read(
  const service_provider & sp,
  dht::network::client * network_client,
  const std::shared_ptr<vds::continuous_buffer<uint8_t>> &input_stream) {
  for (;;) {
    size_t readed = co_await input_stream->read_async(this->buffer_ + this->readed_, sizeof(this->buffer_) - this->readed_);

    if (0 == readed) {
      co_await this->process_data(sp, network_client);
      this->total_hash_.final();
      this->result_hash_ = this->total_hash_.signature();
      co_return;
    }
    else {
      this->readed_ += readed;
      if (this->readed_ == sizeof(this->buffer_)) {
        co_await this->process_data(sp, network_client);
      }
    }
  }
}

std::future<void> vds::_upload_stream_task::process_data(
  const service_provider & sp,
  dht::network::client * network_client) {

  if(0 == this->readed_) {
    co_return;
  }
  else {
    this->total_hash_.update(this->buffer_, this->readed_);
    this->total_size_ += this->readed_;
  }

  co_await sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this(), network_client](
    database_transaction &t)->bool{

    auto block_info = network_client->save(
      sp,
      t,
      vds::const_data_buffer(pthis->buffer_, pthis->readed_));

    pthis->file_blocks_.push_back(transactions::user_message_transaction::file_block_t{
      /*block_id =*/ block_info.id,
      /*block_key =*/ block_info.key,
      /*object_ids*/block_info.object_ids,
      /*block_size =*/ pthis->readed_
    });

    pthis->readed_ = 0;

    return true;
  });
}


