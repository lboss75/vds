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
    template <typename input_stream_type = async_stream<uint8_t>, typename output_stream_type = continuous_stream<uint8_t>>
    inline async_task<> http_pipeline(
      const std::shared_ptr<input_stream_type> & input_stream,
      const std::shared_ptr<async_stream<std::shared_ptr<http_message>>> & input_commands,

      const std::shared_ptr<async_stream<std::shared_ptr<http_message>>> & output_commands,
      const std::shared_ptr<output_stream_type> & output_stream
    )
    {
      return async_series(
        create_async_task(
          [output_commands, output_stream](
            const std::function<void(const service_provider & sp)> & done,
            const error_handler & on_error,
            const service_provider & sp) {
        dataflow(
          stream_read<async_stream<std::shared_ptr<http_message>>>(output_commands),
          http_serializer(),
          stream_write<output_stream_type>(output_stream)
        )(
          [done](const service_provider & sp) {
            sp.get<logger>()->debug("HTTP", sp, "writer closed");
            done(sp);
          },
          [on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.get<logger>()->debug("HTTP", sp, "writer error");
            on_error(sp, ex);
          },
          sp.create_scope("HTTP writer"));
        }),
        create_async_task(
          [input_commands, input_stream](
            const std::function<void(const service_provider & sp)> & done,
            const error_handler & on_error,
            const service_provider & sp) {
        dataflow(
          stream_read(input_stream),
          http_parser(
            [input_commands, done, on_error](const service_provider & sp, const std::shared_ptr<http_message> & request) -> async_task<> {

          if (!request) {
            return input_commands->write_all_async(sp, nullptr, 0);
          }
          else {
            return input_commands->write_value_async(sp, request);
          }
        })
        )(
          [done](const service_provider & sp) {
            sp.get<logger>()->debug("HTTP", sp, "reader closed");
            done(sp);
          },
          [on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.get<logger>()->debug("HTTP", sp, "reader error");
            on_error(sp, ex);
          },
          sp.create_scope("HTTP reader"));
      })
        );
    }
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
