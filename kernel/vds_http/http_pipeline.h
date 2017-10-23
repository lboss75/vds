#ifndef __VDS_HTTP_HTTP_PIPELINE_H_
#define __VDS_HTTP_HTTP_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "mt_service.h"
#include "tcp_network_socket.h"
#include "logger.h"
#include "async_task.h"
#include "http_message.h"
#include "http_serializer.h"
#include "http_parser.h"
#include "async_buffer.h"

namespace vds {
    template <typename input_stream_type = async_buffer<uint8_t>, typename output_stream_type = continuous_buffer<uint8_t>>
    inline async_task<> _copy_http_commands(
      const service_provider & sp,
      const std::shared_ptr<http_serializer> & serializer,
      const std::shared_ptr<std::shared_ptr<http_message>> & buffer,
      const std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> & output_commands)
    {
      return output_commands->read_async(sp, buffer.get(), 1)
      .then([sp, serializer, buffer, output_commands](size_t readed){
        if(0 != readed){
          return serializer->write(sp, *buffer)
          .then([sp, serializer, buffer, output_commands](){
            _copy_http_commands(sp, serializer, buffer, output_commands);
          });
        } else {
          return async_task<>::empty();
        }
      });
    }

    template <typename input_stream_type = async_buffer<uint8_t>, typename output_stream_type = continuous_buffer<uint8_t>>
    inline async_task<> http_pipeline(
      const service_provider & sp,
      
      const std::shared_ptr<input_stream_type> & input_stream,
      const std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> & input_commands,

      const std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> & output_commands,
      const std::shared_ptr<output_stream_type> & output_stream
    )
    {
      auto parser = std::make_shared<http_parser>(
        [sp, input_commands](const std::shared_ptr<http_message> & request) -> async_task<> {
          if (!request) {
            return input_commands->write_all_async(sp, nullptr, 0);
          }
          else {
            return input_commands->write_value_async(sp, request);
          }
      });

      return async_series(
        _copy_http_commands(
          sp,
          std::make_shared<http_serializer>(*output_stream),
          std::make_shared<std::shared_ptr<http_message>>(),
          output_commands),
        copy_stream<uint8_t>(sp, input_stream, parser)
      );
    }
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
