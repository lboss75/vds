
#include "stdafx.h"
#include "private/upload_stream_task_p.h"
#include "chunk_manager.h"
#include "db_model.h"

vds::_upload_stream_task::_upload_stream_task()
    : readed_(0) {
}

vds::async_task<std::list<vds::transactions::file_add_transaction::file_block_t>> vds::_upload_stream_task::start(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream) {

  this->chunk_mng_ = sp.get<chunk_manager>();
  return this->continue_read(sp, input_stream).then([pthis = this->shared_from_this()]() {
    return pthis->file_blocks_;
  });

}

vds::async_task<>
vds::_upload_stream_task::continue_read(
    const service_provider & sp,
    const std::shared_ptr<vds::continuous_buffer<uint8_t>> &input_stream) {
  return input_stream->read_async(this->buffer_ + this->readed_, sizeof(this->buffer_) - this->readed_)
      .then([pthis = this->shared_from_this(), sp, input_stream](size_t readed) -> async_task<>{
        if(0 == readed){
          return pthis->process_data(sp);
        }
        else {
          pthis->readed_ += readed;
          if(pthis->readed_ == sizeof(pthis->buffer_)){
            return pthis->process_data(sp).then([pthis, sp, input_stream]() {
              return pthis->continue_read(sp, input_stream);
            });
          }
          else {
            return pthis->continue_read(sp, input_stream);
          }
        }
      });
}

vds::async_task<> vds::_upload_stream_task::process_data(
  const service_provider & sp) {
  return sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this()](
    database_transaction &t)->bool{

    size_t padding;
    std::unordered_map<uint16_t, const_data_buffer> replica_hashes;
    auto block_info = pthis->chunk_mng_->save_block(
      sp,
      t,
      vds::const_data_buffer(pthis->buffer_, pthis->readed_),
      padding,
      replica_hashes);

    pthis->readed_ = 0;

    pthis->file_blocks_.push_back(transactions::file_add_transaction::file_block_t{
      /*block_id =*/ block_info.id,
      /*block_key =*/ block_info.key,
      /*block_size =*/ safe_cast<uint32_t>(block_info.data.size()),
      /*padding =*/safe_cast<uint16_t>(padding),
      /*replica_hashes =*/replica_hashes
    });

    return true;
  });
}


