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

namespace vds {
    template <typename input_stream_type = async_buffer<uint8_t>, typename output_stream_type = continuous_buffer<uint8_t>>
    inline async_task<> http_pipeline(
      const std::shared_ptr<input_stream_type> & input_stream,
      const std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> & input_commands,

      const std::shared_ptr<async_buffer<std::shared_ptr<http_message>>> & output_commands,
      const std::shared_ptr<output_stream_type> & output_stream
    )
    {
      return async_series(
        dataflow(
          stream_read<async_buffer<std::shared_ptr<http_message>>>(output_commands),
          http_serializer(),
          stream_write<output_stream_type>(output_stream)
        ),
        dataflow(
          stream_read(input_stream),
          http_parser(
            [input_commands](const service_provider & sp, const std::shared_ptr<http_message> & request) -> async_task<> {

          if (!request) {
            return input_commands->write_all_async(sp, nullptr, 0);
          }
          else {
            return input_commands->write_value_async(sp, request);
          }
        })
        )
        );
    }
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
