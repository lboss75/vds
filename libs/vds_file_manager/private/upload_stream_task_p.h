#ifndef __VDS_FILE_MANAGER_UPLOAD_STREAM_TASK_P_H_
#define __VDS_FILE_MANAGER_UPLOAD_STREAM_TASK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>

#include "file_operations.h"
#include "hash.h"
#include "dht_network.h"

namespace vds {
  namespace dht {
    namespace network {
      class client;
    }
  }

  class _upload_stream_task : public std::enable_shared_from_this<_upload_stream_task> {
  public:
    _upload_stream_task();

    vds::async_task<std::list<transactions::user_message_transaction::file_block_t>> start(
        const service_provider * sp,
        const std::shared_ptr<stream_input_async<uint8_t>> & input_stream);

    uint64_t total_size() const {
      return this->total_size_;
    }

    const_data_buffer result_hash() const {
      return this->total_hash_.signature();
    }

  private:
    uint64_t total_size_;
    hash total_hash_;

    uint64_t readed_;
    uint8_t buffer_[1024];
  };
}

#endif //__VDS_FILE_MANAGER_UPLOAD_STREAM_TASK_P_H_

