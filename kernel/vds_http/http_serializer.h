#ifndef __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
#define __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_stream.h"
#include "http_message.h"
#include "http_request.h"
#include "json_object.h"

namespace vds {
  class http_serializer
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
        const http_serializer & args)
        : base_class(context), state_(StateEnum::STATE_BOF)
      {
      }

      void async_process_data(const service_provider & sp)
      {
        if (0 == this->input_buffer_size()) {
          this->state_ = StateEnum::STATE_EOF;
          this->processed(sp, 0, 0);
          return;
        }
        
        if(StateEnum::STATE_BOF == this->state_){
          this->state_ = StateEnum::STATE_BODY;

          auto message = this->input_buffer(0);
          mt_service::async(sp, [this, sp, message]() {

            std::stringstream stream;
            for (auto & header : message->headers()) {
              stream << header << "\n";
            }
            stream << "\n";

            auto data = std::make_shared<std::string>(stream.str());
            this->buffer_.write_all_async(sp, (const uint8_t *)data->c_str(), data->length()).wait(
              [this, data, message](const service_provider & sp) {
              auto buffer = std::make_shared<std::vector<uint8_t>>(1024);
              this->write_body(sp, message, buffer);
            },
              [this, data, message](const service_provider & sp, std::exception_ptr ex) {
              this->error(sp, ex);
            },
              sp);
          });
        }
        
        this->continue_process(sp);
      }

    private:
      enum class StateEnum
      {
        STATE_BOF,
        STATE_BODY,
        STATE_EOF
      };
      StateEnum state_;
      async_stream<uint8_t> buffer_;

      void write_body(
        const service_provider & sp,
        const std::shared_ptr<http_message> & message,
        const std::shared_ptr<std::vector<uint8_t>> & buffer)
      {
        message->body()->read_async(sp, buffer->data(), buffer->size())
          .wait(
            [this, message, buffer](const service_provider & sp, size_t readed) {
          if (0 < readed) {
            this->buffer_.write_all_async(sp, buffer->data(), readed).wait(
              [this, message, buffer](const service_provider & sp) {
              this->write_body(sp, message, buffer);
            },
              [this](const service_provider & sp, std::exception_ptr ex) {
              this->error(sp, ex);
            },
              sp);
          }
          else {
            this->buffer_.write_all_async(sp, nullptr, 0).wait(
              [](const service_provider & sp) { },
              [](const service_provider & sp, std::exception_ptr ex) {},
              sp);
          }
        }, [this](const service_provider & sp, std::exception_ptr ex) {
          this->error(sp, ex);
        },
          sp
          );
      }

      void continue_process(const service_provider & sp)
      {
        this->buffer_.read_async(sp, this->output_buffer(), this->output_buffer_size()).wait(
          [this](const service_provider & sp, size_t readed) {

          if (0 < readed) {
            if (this->processed(sp, 0, readed)) {
              this->continue_process(sp);
            }
          }
          else {
            this->state_ = StateEnum::STATE_BOF;
            this->buffer_.reset();
            if (this->processed(sp, 1, 0)) {
              this->async_process_data(sp);
            }
          }
        },
          [this](const service_provider & sp, std::exception_ptr ex) {
          this->error(sp, ex);
        },
          sp);
      }
    };
  };

  class json_to_http_channel
  {
  public:
    json_to_http_channel(
      const std::string & method,
      const std::string & url)
    : method_(method), url_(url)
    {
    }

    using incoming_item_type = std::shared_ptr<json_value>;
    using outgoing_item_type = std::shared_ptr<http_message>;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public sync_dataflow_filter<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_filter<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const json_to_http_channel & args)
        : base_class(context), 
          method_(args.method_),
          url_(args.url_)
      {
      }

      void sync_process_data(const vds::service_provider & sp, size_t & input_readed, size_t & output_written)
      {
        if (0 < this->input_buffer_size()) {
          if (0 == this->output_buffer_size()) {
            throw std::runtime_error("Logic error");
          }

          auto message = std::make_unique<json_array>();
          for (size_t i = 0; i < this->input_buffer_size(); ++i) {
            message->add(this->input_buffer(i));
          }

          this->output_buffer(0) = http_request::simple_request(sp, this->method_, this->url_, message->json_value::str());
          output_written = 1;
        }
        else {
          output_written = 0;
        }
        input_readed = this->input_buffer_size();        
      }
    private:
      std::string method_;
      std::string url_;
    };
  private:
    std::string method_;
    std::string url_;
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
