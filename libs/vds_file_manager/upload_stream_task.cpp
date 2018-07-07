
#include "stdafx.h"
#include "private/upload_stream_task_p.h"
#include "db_model.h"
#include "dht_network_client.h"

vds::_upload_stream_task::_upload_stream_task()
: total_hash_(hash::sha256()), total_size_(0), readed_(0) {
}

vds::async_task<std::list<vds::transactions::file_add_transaction::file_block_t>> vds::_upload_stream_task::start(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream) {

  auto network_client = sp.get<dht::network::client>();
  return this->continue_read(sp, network_client, input_stream).then([pthis = this->shared_from_this()]() {
    return pthis->file_blocks_;
  });

}

vds::async_task<>
vds::_upload_stream_task::continue_read(
    const service_provider & sp,
    dht::network::client * network_client,
    const std::shared_ptr<vds::continuous_buffer<uint8_t>> &input_stream) {
  return input_stream->read_async(this->buffer_ + this->readed_, sizeof(this->buffer_) - this->readed_)
      .then([pthis = this->shared_from_this(), sp, input_stream, network_client](size_t readed) -> async_task<>{
        if(0 == readed){
          return pthis->process_data(sp, network_client).then([pthis]() {
            pthis->total_hash_.final();
            pthis->result_hash_ = pthis->total_hash_.signature();
          });
        }
        else {
          pthis->readed_ += readed;
          if(pthis->readed_ == sizeof(pthis->buffer_)){
            return pthis->process_data(sp, network_client).then([pthis, sp, input_stream, network_client]() {
              return pthis->continue_read(sp, network_client, input_stream);
            });
          }
          else {
            return pthis->continue_read(sp, network_client, input_stream);
          }
        }
      });
}

vds::async_task<> vds::_upload_stream_task::process_data(
  const service_provider & sp,
  dht::network::client * network_client) {

  if(0 == this->readed_) {
    return async_task<>::empty();
  }
  else {
    this->total_hash_.update(this->buffer_, this->readed_);
    this->total_size_ += this->readed_;
  }

  return sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this(), network_client](
    database_transaction &t)->bool{

    auto block_info = network_client->save(
      sp,
      t,
      vds::const_data_buffer(pthis->buffer_, pthis->readed_));

    pthis->file_blocks_.push_back(transactions::file_add_transaction::file_block_t{
      /*block_id =*/ block_info.id,
      /*block_key =*/ block_info.key,
      /*object_ids*/block_info.object_ids,
      /*block_size =*/ pthis->readed_
    });

    pthis->readed_ = 0;

    return true;
  });
}


