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

  class _upload_stream_task : public stream_output_async<uint8_t> {
  public:
    _upload_stream_task(
      const service_provider * sp,
      lambda_holder_t<
        async_task<expected<void>>,
        const const_data_buffer & /*result_hash*/,
        uint64_t /*total_size*/,
        std::list<transactions::user_message_transaction::file_block_t> &&> result_handler);

    async_task<expected<void>> write_async(
      const uint8_t *data,
      size_t len) override;

    uint64_t total_size() const {
      return this->total_size_;
    }

    const_data_buffer result_hash() const {
      return this->total_hash_.signature();
    }

    void set_file_hash(const const_data_buffer & file_hash) {
      this->target_file_hash_ = file_hash;
    }

  private:
    const service_provider * sp_;
    uint64_t total_size_;
    hash total_hash_;
    size_t readed_;

    std::shared_ptr<stream_output_async<uint8_t>> current_target_;
    
    const_data_buffer target_file_hash_;
 
    std::list<transactions::user_message_transaction::file_block_t> result_;
    lambda_holder_t<
      async_task<expected<void>>,
      const const_data_buffer & /*result_hash*/,
      uint64_t /*total_size*/,
      std::list<transactions::user_message_transaction::file_block_t> &&> result_handler_;
  };
}

#endif //__VDS_FILE_MANAGER_UPLOAD_STREAM_TASK_P_H_

