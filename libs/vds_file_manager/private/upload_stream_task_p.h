#ifndef __VDS_FILE_MANAGER_UPLOAD_STREAM_TASK_P_H_
#define __VDS_FILE_MANAGER_UPLOAD_STREAM_TASK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "async_task.h"
#include "file_operations.h"

namespace vds {
  class _upload_stream_task : public std::enable_shared_from_this<_upload_stream_task> {
  public:
    _upload_stream_task();

    async_task<> start(
        const service_provider & sp,
        database_transaction &t,
        const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream,
        std::list<transactions::file_add_transaction::file_block_t> &file_blocks);

  private:
    uint8_t buffer_[vds::file_manager::file_operations::BLOCK_SIZE];
    size_t readed_;
    class chunk_manager *chunk_mng_;

    async_task<> continue_read(
        const service_provider & sp,
        database_transaction &t,
        const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream,
        std::list<transactions::file_add_transaction::file_block_t> &file_blocks);

    transactions::file_add_transaction::file_block_t process_data(
        const service_provider & sp,
        database_transaction &t);
  };
}

#endif //__VDS_FILE_MANAGER_UPLOAD_STREAM_TASK_P_H_

