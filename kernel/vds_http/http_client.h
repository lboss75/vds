#ifndef __VDS_HTTP_HTTP_CLIENT_H_
#define __VDS_HTTP_HTTP_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "async_buffer.h"

namespace vds {
  class http_message;

  class http_client
  {
  public:
    typedef std::function<vds::async_task<void> (const std::shared_ptr<vds::http_message> & request)> handler_type;

    http_client();

    vds::async_task<void> start(
      
      const handler_type & handler);

    vds::async_task<void> send(
      
      const std::shared_ptr<vds::http_message> & message);

  private:
    handler_type handler_;

    vds::async_task<void> process_input_commands(
      
      const handler_type & handler);
  };
}

#endif // __VDS_HTTP_HTTP_CLIENT_H_
