#include "http_client.h"
#include "logger.h"
#include "http_pipeline.h"

vds::http_client::http_client()
: input_commands_(new vds::async_buffer<std::shared_ptr<http_message>>()),
  output_commands_(new vds::async_buffer<std::shared_ptr<http_message>>())
{
}

vds::async_task<> vds::http_client::send(
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


vds::async_task<> vds::http_client::start(
  const vds::service_provider & sp,
  const std::shared_ptr<continuous_buffer<uint8_t>> & incoming_stream,
  const std::shared_ptr<continuous_buffer<uint8_t>> & outgoing_stream,
  const handler_type & handler)
{
  return async_series(
    http_pipeline(sp, incoming_stream, this->input_commands_, this->output_commands_, outgoing_stream),
    this->process_input_commands(sp, handler)
  );
}

vds::async_task<> vds::http_client::process_input_commands(
  const vds::service_provider & sp,
  const handler_type & handler)
{
  return this->input_commands_->read_async(sp, &this->input_buffer_, 1)
    .then([this, sp, handler](size_t readed) {
    if (0 == readed) {
      return handler(std::shared_ptr<vds::http_message>());
    }
    else {
      return handler(this->input_buffer_)
        .then([this, sp, handler]() {
        return this->process_input_commands(sp, handler);
      });
    }
  });
}
