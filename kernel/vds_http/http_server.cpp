#include "http_server.h"
#include "logger.h"
#include "http_pipeline.h"

vds::http_server::http_server()
  : input_commands_(new vds::async_buffer<std::shared_ptr<http_message>>()),
  output_commands_(new vds::async_buffer<std::shared_ptr<http_message>>())
{
}

vds::async_task<> vds::http_server::send(
  const vds::service_provider & sp,
  const std::shared_ptr<vds::http_message>& message)
{
  if (!message) {
    return this->output_commands_->write_all_async(sp, nullptr, 0);
  }
  else {
    return this->output_commands_->write_value_async(sp, message);
  }
}


vds::async_task<> vds::http_server::start(
  const vds::service_provider & sp,
  const std::shared_ptr<continuous_buffer<uint8_t>> & incoming_stream,
  const std::shared_ptr<continuous_buffer<uint8_t>> & outgoing_stream,
  const handler_type & handler)
{
  return async_series(
    http_pipeline(incoming_stream, this->input_commands_, this->output_commands_, outgoing_stream),
    dataflow(
      stream_read(this->input_commands_),
      dataflow_consumer<std::shared_ptr<http_message>>(
        [this, handler](
          const vds::service_provider & sp,
          const std::shared_ptr<vds::http_message> * requests,
          size_t count) -> vds::async_task<size_t> {
          return create_async_task(
            [this, handler, requests, count](
              const std::function<void(const vds::service_provider & sp, size_t readed)> & task_done,
              const vds::error_handler & on_error,
              const vds::service_provider & sp)
          {
            if (0 == count) {
              this->send(sp, std::shared_ptr<vds::http_message>())
                .wait([task_done](const vds::service_provider & sp) {
                  task_done(sp, 0);
                  },
                  on_error,
                  sp);
            }
            else {
              return handler(sp, requests[0]).wait(
                [this, task_done, on_error](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & response) {
                  this->send(sp, response).wait([task_done](const vds::service_provider & sp) {
                    task_done(sp, 1);
                    },
                    on_error,
                    sp);
                },
                on_error,
                sp);
            }
          });
        }
      )
    )
  );
}
