#ifndef __VDS_HTTP_HTTP_SERVER_P_H_
#define __VDS_HTTP_HTTP_SERVER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_server.h"
#include "http_pipeline.h"

namespace vds {
  class _http_server : public std::enable_shared_from_this<_http_server>
  {
  public:
    _http_server()
      : input_commands_(new vds::async_buffer<std::shared_ptr<http_message>>()),
        output_commands_(new vds::async_buffer<std::shared_ptr<http_message>>())
      {
      }

    async_task<> start(
      const vds::service_provider & sp,
      const std::shared_ptr<_http_server> & pthis,
      const std::shared_ptr<continuous_buffer<uint8_t>> & incoming_stream,
      const std::shared_ptr<continuous_buffer<uint8_t>> & outgoing,
      const http_server::handler_type & handler)
    {
      return async_series(
        http_pipeline(sp, incoming_stream, this->input_commands_, this->output_commands_, outgoing),
        this->process_input(sp, pthis, handler)
      );
    }

  private:
    http_server::handler_type handler_;
    std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> input_commands_;
    std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> output_commands_;
    std::shared_ptr<http_message> input_buffer_;

    async_task<> send(
      const vds::service_provider & sp,
      const std::shared_ptr<vds::http_message> & message)
    {
      if (!message) {
        return this->output_commands_->write_all_async(sp, nullptr, 0);
      }
      else {
        return this->output_commands_->write_value_async(sp, message);
      }
    }
    
    async_task<> process_input(
      const vds::service_provider & sp,
      const std::shared_ptr<_http_server> & pthis,
      const http_server::handler_type & handler)
    {
      return this->input_commands_->read_async(sp, &this->input_buffer_, 1)
      .then([pthis, sp, handler](size_t count) {
          if (0 == count) {
            return pthis->send(sp, std::shared_ptr<vds::http_message>());
          }
          else {
            return handler(pthis->input_buffer_)
            .then([pthis, sp, handler](const std::shared_ptr<vds::http_message> & response) {
              return pthis->send(sp, response)
                .then([pthis, sp, handler]() {
                  return pthis->process_input(sp, pthis, handler);
                });
            });
          }
      });
    }
  };

}

#endif // __VDS_HTTP_HTTP_SERVER_P_H_
