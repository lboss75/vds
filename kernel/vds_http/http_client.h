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
    typedef std::function<async_task<> (const std::shared_ptr<vds::http_message> & request)> handler_type;

    http_client();

    async_task<> start(
      const vds::service_provider & sp,
      const std::shared_ptr<continuous_buffer<uint8_t>> & incoming_stream,
      const std::shared_ptr<continuous_buffer<uint8_t>> & outgoing_stream,
      const handler_type & handler);

    async_task<> send(
      const vds::service_provider & sp,
      const std::shared_ptr<vds::http_message> & message);
    
    const std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> & output_commands() const {
      return this->output_commands_;
    }

  private:
    handler_type handler_;
    std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> input_commands_;
    std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> output_commands_;

    std::shared_ptr<http_message> input_buffer_;
    async_task<> process_input_commands(
      const vds::service_provider & sp,
      const handler_type & handler);

  };
}

#endif // __VDS_HTTP_HTTP_CLIENT_H_
