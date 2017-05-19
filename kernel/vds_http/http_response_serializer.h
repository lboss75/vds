#ifndef __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
#define __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_stream.h"
#include "http_message.h"

namespace vds {
  class http_response_serializer
  {
  public:

    using incoming_item_type = std::shared_ptr<http_message>;
    using outgoing_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public async_dataflow_filter<context_type, handler<context_type>>
    {
      using base_class = async_dataflow_filter<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const http_response_serializer & args)
        : base_class(context), state_(STATE_BOF)
      {
      }

      void async_process_data(const service_provider & sp)
      {
        std::lock_guard<std::mutex> lock(this->state_mutex_);

        this->continue_process(sp);
      }
      
      void processed(const service_provider & sp)
      {
        if (!this->stream_ || !this->stream_->read(sp, this->next)) {
          this->prev(sp);
        }
      }

    private:
      enum class StateEnum {
        STATE_BOF,
        STATE_WRITE_HEADERS,
        STATE_EOF
      };

      std::mutex state_mutex_;
      StateEnum state_;
      async_stream buffer_;

      void continue_process(const service_provider & sp)
      {
        for (;;) {
          switch (this->state_) {
          case BOF:
            this->state_ = STATE_WRITE_HEADERS;
            auto message = this->input_buffer_[0];

            std::stringstream stream;
            for (auto & header : message.headers()) {
              stream << header << "\n";
            }

            stream << "Content-Length: " << message.body().length() << "\n\n";
            stream << message.body();

            auto data = std::make_shared<std::string>(stream.str());
            this->buffer_.write_async(data.get(), data->length()).wait(
              [data, this](const service_provider & sp) {
                std::lock_guard<std::mutex> lock(this->state_mutex_);
                if (this->processed(sp, 1, 0)) {
                  this->continue_process(sp);
                }
              },
              [data, this](const service_provider & sp, std::exception_ptr ex) {
                this->on_error(sp, ex);
              },
              sp);
            break;
          case WRITE_HEADERS:
              buffer_.read_async(sp, this->output_buffer_, this->output_buffer_size_).wait(
                [this](const service_provider & sp, size_t readed) {

                std::lock_guard<std::mutex> lock(this->state_mutex_);
                if (0 < readed) {
                  if (this->processed(sp, 0, readed)) {
                    this->continue_process(sp);
                  }
                }
                else {
                  this->state_ = STATE_BOF;
                  if (this->processed(sp, 1, 0)) {
                    this->continue_process(sp);
                  }
                }
              },
              [this](const service_provider & sp, std::exception_ptr ex) {
                this->on_error(sp, ex);
              },
              sp);
            return;
            break;
          }
        }
      }
    };
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
