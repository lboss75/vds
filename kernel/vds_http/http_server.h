#ifndef __VDS_HTTP_HTTP_SERVER_H_
#define __VDS_HTTP_HTTP_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "async_stream.h"

namespace vds {
  class http_message;

  class http_server
  {
  public:
    typedef std::function<async_task<std::shared_ptr<vds::http_message>> (const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & request)> handler_type;

    http_server();

    async_task<> start(
      const vds::service_provider & sp,
      const std::shared_ptr<continuous_stream<uint8_t>> & incoming_stream,
      const std::shared_ptr<continuous_stream<uint8_t>> & outgoing,
      const handler_type & handler);

  private:
    handler_type handler_;
    std::shared_ptr<async_stream<std::shared_ptr<http_message>>> input_commands_;
    std::shared_ptr<async_stream<std::shared_ptr<http_message>>> output_commands_;

    async_task<> send(
      const vds::service_provider & sp,
      const std::shared_ptr<vds::http_message> & message);
  };
}

#endif // __VDS_HTTP_HTTP_SERVER_H_
