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

    std::future<std::list<transactions::user_message_transaction::file_block_t>> start(
        const service_provider * sp,
        const std::shared_ptr<stream_input_async<uint8_t>> & input_stream);

    const const_data_buffer & result_hash() const {
      return this->result_hash_;
    }

    size_t total_size() const {
      return this->total_size_;
    }

  private:
    hash total_hash_;
    size_t total_size_;

    uint8_t buffer_[vds::dht::network::service::BLOCK_SIZE];
    size_t readed_;
    std::list<transactions::user_message_transaction::file_block_t> file_blocks_;

    const_data_buffer result_hash_;

    std::future<void> continue_read(
      const service_provider * sp,
        dht::network::client * network_client,
        const std::shared_ptr<stream_input_async<uint8_t>> & input_stream);

    std::future<void> process_data(
        const service_provider * sp,
        dht::network::client * network_client);
  };
}

#endif //__VDS_FILE_MANAGER_UPLOAD_STREAM_TASK_P_H_

