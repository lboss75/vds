#ifndef __VDS_USER_MANAGER_USER_STORAGE_H_
#define __VDS_USER_MANAGER_USER_STORAGE_H_

#include "json_object.h"
#include "async_task.h"
#include "const_data_buffer.h"
#include "transaction_block_builder.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class iserver_api {
  public:
    struct data_info_t {
      const_data_buffer data_hash;
      uint32_t replica_size;
      std::vector<const_data_buffer> replicas;
    };

    virtual async_task<expected<data_info_t>> upload_data(const const_data_buffer& data) = 0;
    virtual async_task<expected<const_data_buffer>> broadcast(const const_data_buffer& data) = 0;
  };
}

#endif // __VDS_USER_MANAGER_USER_STORAGE_H_
