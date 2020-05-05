#ifndef __VDS_SEVER_SERVER_API_H_
#define __VDS_SEVER_SERVER_API_H_

#include "json_object.h"
#include "async_task.h"
#include "const_data_buffer.h"
#include "transaction_block_builder.h"
#include "iserver_api.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class server_api : public iserver_api {
  public:
    server_api(const service_provider* sp)
      : sp_(sp)
    {}

    async_task<expected<data_info_t>> upload_data(const const_data_buffer& data) override;
    async_task<expected<const_data_buffer>> broadcast(const const_data_buffer& data, bool allow_root) override;

  private:
    const service_provider* sp_;
  };
}

#endif // __VDS_SEVER_SERVER_API_H_
