
#include "stdafx.h"
#include "private/upload_stream_task_p.h"
#include "chunk_manager.h"

vds::_upload_stream_task::_upload_stream_task()
    : readed_(0) {
}

vds::async_task<> vds::_upload_stream_task::start(
    const service_provider & sp,
    vds::database_transaction &t,
    const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream,
    std::list<transactions::file_add_transaction::file_block_t> &file_blocks) {

  this->chunk_mng_ = sp.get<chunk_manager>();
  return this->continue_read(sp, t, input_stream, file_blocks);

}

vds::async_task<>
vds::_upload_stream_task::continue_read(
    const service_provider & sp,
    database_transaction &t,
    const std::shared_ptr<vds::continuous_buffer<uint8_t>> &input_stream,
    std::list<transactions::file_add_transaction::file_block_t> &file_blocks) {
  return input_stream->read_async(this->buffer_ + this->readed_, sizeof(this->buffer_) - this->readed_)
      .then([pthis = this->shared_from_this(), sp, input_stream, &file_blocks, &t](size_t readed) -> async_task<>{
        if(0 == readed){
          file_blocks.push_back(pthis->process_data(sp, t));
          return vds::async_task<>::empty();
        }
        else {
          pthis->readed_ += readed;
          if(pthis->readed_ == sizeof(pthis->buffer_)){
            file_blocks.push_back(pthis->process_data(sp, t));
          }

          return pthis->continue_read(sp, t, input_stream, file_blocks);
        }
      });
}

vds::transactions::file_add_transaction::file_block_t vds::_upload_stream_task::process_data(
    const service_provider & sp,
    vds::database_transaction &t) {

  size_t padding;
  std::unordered_map<uint16_t, const_data_buffer> replica_hashes;
  auto block_info = this->chunk_mng_->save_block(
      sp,
      t,
      vds::const_data_buffer(this->buffer_, this->readed_),
      padding,
      replica_hashes);

  this->readed_ = 0;

  return transactions::file_add_transaction::file_block_t{
      /*block_id =*/ block_info.id,
      /*block_key =*/ block_info.key,
      /*block_size =*/ block_info.data.size(),
      /*padding =*/padding,
      /*replica_hashes =*/replica_hashes
  };
}


