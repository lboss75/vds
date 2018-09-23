#ifndef __VDS_HTTP_HTTP_SERVER_H_
#define __VDS_HTTP_HTTP_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "async_buffer.h"

namespace vds {
  class http_message;
  class _http_server;

  class http_server
  {
  public:
    typedef std::function<vds::async_task<std::shared_ptr<vds::http_message>>(const std::shared_ptr<vds::http_message> & request)> handler_type;

    http_server();
    ~http_server();

    vds::async_task<void> start(
      const vds::service_provider & sp,
      const std::shared_ptr<continuous_buffer<uint8_t>> & incoming_stream,
      const std::shared_ptr<continuous_buffer<uint8_t>> & outgoing,
      const handler_type & handler);

  private:
    std::shared_ptr<_http_server> impl_;
  };
}

#endif // __VDS_HTTP_HTTP_SERVER_H_
